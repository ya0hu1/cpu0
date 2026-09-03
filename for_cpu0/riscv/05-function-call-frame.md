# RiscvToy Stage 5：函数调用与栈帧

## 1. 为什么必须进入 Stage 5

Stage 4 的 RiscvToy 已经能把最简单的算术函数打印成：

```asm
add:
  add a0, a0, a1
  ret
```

但这样的后端还不能编译真实程序，因为它缺少两类能力：

1. 访存指令：`alloca`、局部变量、寄存器溢出都依赖 `load/store`；
2. 函数调用：`call` 会写入返回地址寄存器 `x1/ra`，所以函数如果调用了别的函数，
   就必须先把进入本函数时的 `ra` 保存到栈上。

RISC-V 约定中，`sp` 向下增长，`ra` 没有专门的硬件保存机制。编译器只能在函数开头
分配栈空间并保存 `ra`，在返回前恢复 `ra` 并释放栈空间。

## 2. 本阶段新增/修改的关键文件

```text
backend/RiscvToy/
  RiscvToyInstrFormats.td     # 增加 S 型指令格式
  RiscvToyInstrInfo.td        # 增加 lw/sw、call 相关伪指令
  RiscvToyRegisterInfo.td     # 调整 GPR 类，允许指令引用 x1/x2/x3/x4
  RiscvToyISelDAGToDAG.cpp    # 实现 SelectAddrFI()
  RiscvToyInstrInfo.{h,cpp}   # 栈槽 load/store 与寄存器溢出
  RiscvToyRegisterInfo.cpp    # 实现 eliminateFrameIndex()
  RiscvToyFrameLowering.cpp   # 实现 prologue/epilogue
  RiscvToyISelLowering.{h,cpp} # 实现 LowerCall()/LowerCallResult()
  RiscvToyAsmPrinter.cpp      # 打印 call 符号
```

## 3. 访存指令

### 3.1 `lw` 和 `sw`

RV32I 访存指令使用寄存器加 12 位有符号偏移：

```text
lw rd, offset(rs1)
sw rs2, offset(rs1)
```

`lw` 是 I 型编码，`sw` 是 S 型编码。S 型格式和 I 型不同：它的 12 位立即数被拆成
两个字段放在指令的两端。RiscvToy 在 `RiscvToyInstrFormats.td` 中新加了
`RiscvToyS` 类：

```tablegen
class RiscvToyS<bits<3> funct3, bits<7> opcode, ...> {
  bits<12> imm12;
  bits<5> rs2;
  bits<5> rs1;

  let Inst{31-25} = imm12{11-5};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7}  = imm12{4-0};
  let Inst{6-0}   = opcode;
}
```

### 3.2 为什么机器指令的寄存器类要改

Stage 2 中普通可分配类 `GPR` 刻意排除了 `x0/x2/x3/x4`，但也没有包含 `x1`。
到了 Stage 5，问题暴露了：

- `x1` 是返回地址，必须在函数调用中保存和恢复；
- `sp` 是栈基址，`sw ra, offset(sp)` 必须能显式写 `x2`；
- 栈槽溢出是通用框架生成的，必须允许 `sw/lw` 操作这些特殊用途寄存器。

所以本阶段把 `GPR` 调整为：

```text
a0-a7, t0-t2, t3-t6, s0-s1, s2-s11, x4, x3, x2, x1
```

其中 `x2/x3/x4` 仍然在 C++ 的 `getReservedRegs()` 中标记为保留，分配器不会把普通值
放进它们。`x1` 在 RISC-V 里本身是 callee-saved 的，放入 `GPR` 和上游做法一致。

## 4. FrameIndex 怎么变成 `sp + offset`

LLVM SelectionDAG 遇到 `alloca` 时不会直接给出一个普通寄存器地址，而是给一个
`FrameIndex`。这种节点不能直接匹配 `GPR`，所以 TableGen 里定义了一个复杂地址：

```tablegen
def AddrFI : ComplexPattern<i32, 1, "SelectAddrFI", [frameindex], []>;
```

指令选择器里的 `SelectAddrFI()` 只做一件事：把 `FrameIndexSDNode` 转成
`TargetFrameIndex`。这样 `store i32 %a, i32* %p` 就能匹配成：

```text
sw %a, 0(FrameIndex)
```

真正的地址计算发生在 PrologEpilogInserter（PEI）阶段。PEI 算完每个栈对象的偏移后，
调用：

```text
RiscvToyRegisterInfo::eliminateFrameIndex()
```

当前 RiscvToy 不使用 frame pointer，所以基址就是 `sp`。默认参考公式为：

```text
final offset = objectOffset + stackSize + instructionImmediate
```

例如一个调用者栈帧大小为 16，`ra` 的槽对象偏移是 `-4`，那么恢复指令会从：

```text
-4 + 16 = 12
```

得到 `lw ra, 12(sp)`。

## 5. Prologue 和 Epilogue

通用 PEI 会先插入“保存 callee-saved 寄存器”的指令，再调用
`emitPrologue()`。因此 RiscvToy 的 `emitPrologue()` 在基本块开头插入
`addi sp, sp, -StackSize`，这条指令会被插到保存指令之前。

最终顺序为：

```asm
addi sp, sp, -16      # 1. 分配栈
sw ra, 12(sp)         # 2. 保存返回地址
...
call callee           # 3. 函数调用
...
lw ra, 12(sp)         # 4. 恢复返回地址
addi sp, sp, 16       # 5. 释放栈
ret
```

Epilogue 则插入在 return terminator 之前。由于通用恢复指令也插在 return 之前，
所以最终顺序是“先恢复寄存器，再恢复 sp”，也就是上面的 `lw ra` 后接 `addi sp`。

`ADDI` 的立即数只有 12 位，单次最多表示 2047。为支持较大栈帧，
`RiscvToyFrameLowering.cpp` 把栈调整切成多个 2047：

```asm
addi sp, sp, -2047
addi sp, sp, -961
sw a0, 0(sp)
addi sp, sp, 2047
addi sp, sp, 961
ret
```

## 6. 函数调用链

### 6.1 LowerCall 的主线

`RiscvToyTargetLowering::LowerCall()` 的流程：

1. 用 `CC_RiscvToy_ILP32` 分析参数；
2. 若参数需要放到栈上，或调用是 varargs，则明确报不支持；
3. 生成 `CALLSEQ_START`，在机器码中对应 `ADJCALLSTACKDOWN`；
4. 把每个参数通过 `CopyToReg` 放入 `a0-a7`；
5. 把 `GlobalAddress/ExternalSymbol` 转成 target 地址；
6. 生成 `RiscvToyISD::CALL`，Instruction Selection 把它匹配成 `PseudoCALL`；
7. 生成 `CALLSEQ_END`，对应 `ADJCALLSTACKUP`；
8. `LowerCallResult()` 从 `a0` 读回返回值。

`CALLSEQ_START/END` 这两条伪指令表示“这一段是调用参数区”。RiscvToy 当前没有栈上传参，
因此它们不带栈空间，PEI 会直接消掉，不需要在调用点手动改 `sp`。

### 6.2 `PseudoCALL` 如何定义

教学阶段没有立即实现 MC 层的 `auipc + jalr` 扩展，而是保留一个可打印的伪指令：

```tablegen
let isCall = 1,
    Defs = [X1, X5, X6, X7, X10, X11, X12, X13, X14, X15,
            X16, X17, X28, X29, X30, X31],
    isCodeGenOnly = 1 in
def PseudoCALL : RiscvToyPseudo<(outs), (ins call_symbol:$dst),
                                "call\t$dst", []>;
```

`Defs` 标记了 RISC-V caller-saved 寄存器，告诉寄存器分配器调用会破坏这些寄存器。
`ra` 在其中，所以 PEI 会在本函数需要保存它时生成 `sw ra`/`lw ra`。

当前 `-filetype=asm` 下由 `RiscvToyAsmPrinter` 把 `PseudoCALL` 的符号 operand
转成 MC 表达式并打印：

```asm
call callee
```

如果要输出真正的 object 文件，将来需要把这个伪指令展开成能编码的跳转序列，这正是
路线图中 Stage 7 的 MC 层工作。

### 6.3 输出示例

输入：

```llvm
define i32 @caller(i32 %a, i32 %b) {
  %r = call i32 @callee(i32 %a, i32 %b)
  ret i32 %r
}
```

输出：

```asm
caller:
  addi sp, sp, -16
  sw ra, 12(sp)
  call callee
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
```

## 7. 验证方式

本阶段新增三个 lit 测试：

```text
backend/test/CodeGen/RiscvToy/04-stack-frame.ll
backend/test/CodeGen/RiscvToy/05-function-call.ll
backend/test/CodeGen/RiscvToy/06-large-frame.ll
```

运行 RiscvToy 的测试：

```bash
build/bin/llvm-lit -sv \
  third_party/llvm-project/llvm/test/CodeGen/RiscvToy
```

也可以直接看汇编：

```bash
build/bin/llc -O0 -march=riscvtoy \
  -mtriple=riscvtoy-unknown-unknown -filetype=asm \
  backend/test/CodeGen/RiscvToy/04-stack-frame.ll
```

## 8. 当前边界

Stage 5 已经补上：

- `lw/sw`；
- `alloca` 和局部栈对象；
- callee-saved 寄存器溢出/恢复；
- `ra` 保存/恢复；
- 直接函数调用；
- 最多 8 个寄存器参数；
- 超过 12 位偏移的大栈帧可以分段调整 `sp`。

还没有支持：

- 第 9 个及以后的栈上传参；
- varargs；
- 间接函数调用；
- aggregate / sret / 复杂返回值；
- `-filetype=obj` 的真实机器码编码；
- 单条访存指令偏移超过 12 位时的基址展开。

这些限制正好为 Stage 6（分支比较）和 Stage 7（完整 MC 层）留出下一批教学问题。
