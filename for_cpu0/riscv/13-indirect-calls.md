# RiscvToy Stage 13：间接函数调用

## 1. 本阶段目标

Stage 5 的函数调用只支持：

```text
call callee
```

也就是 `jal x1, target`。这种调用要求目标地址在编译/链接时能表达成符号，函数指针
还无法使用：

```llvm
%r = call i32 %f(i32 %x)
```

本阶段加入间接调用，让 C 函数指针可以走：

```asm
jalr ra, 0(a2)
```

## 2. 直接调用和间接调用的区别

直接调用：

```text
call callee
=> jal x1, callee
```

`callee` 最终变成 `R_RISCV_JAL` relocation，由链接器填到跳转字段。

间接调用：

```text
函数地址已经在 a2 寄存器里
=> jalr x1, 0(a2)
```

`jalr` 从寄存器读取目标地址，不需要 relocation。RISC-V 的 `jalr rd, offset(rs1)`
会：

```text
rd = pc + 4
pc = rs1 + offset
```

所以 `jalr ra, 0(fnreg)` 保存返回地址到 `ra`，再跳到函数指针。

## 3. 新增 PseudoCALLIndirect

RiscvToy 增加一个 CodeGen 专用伪指令：

```tablegen
let isCall = 1,
    Defs = [X1, X5, X6, X7, X10, X11, X12, X13, X14, X15,
            X16, X17, X28, X29, X30, X31],
    isCodeGenOnly = 1 in
def PseudoCALLIndirect : RiscvToyPseudo<(outs), (ins GPR:$rs1),
                                         "jalr\tra, 0($rs1)", []>;
```

它和 `PseudoCALL` 一样：

- 标记 `isCall`；
- 声明 caller-saved 寄存器被破坏；
- 不参与真实机器码。

TableGen pattern：

```tablegen
def : Pat<(RiscvToyCallFlag GPR:$rs1),
          (PseudoCALLIndirect GPR:$rs1)>;
```

这样 RiscvToyISD::CALL 的 operand 如果是一个普通 GPR 值，就会匹配成间接调用。

## 4. LowerCall 的修改

原先 `LowerCall()` 只接受两类 callee：

```text
GlobalAddress   -> 全局函数
ExternalSymbol  -> 外部函数
```

其他情况会：

```text
report_fatal_error("RiscvToy indirect calls are not supported yet")
```

现在把函数指针当作普通 i32 operand 保留：

```cpp
if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee))
  Callee = DAG.getTargetGlobalAddress(...);
else if (ExternalSymbolSDNode *E = dyn_cast<ExternalSymbolSDNode>(Callee))
  Callee = DAG.getTargetExternalSymbol(...);
// 否则 Callee 继续是普通 i32，之后匹配 PseudoCALLIndirect。
```

## 5. MCCodeEmitter 展开

object 输出时不能保留 pseudo，需要把它展开成真指令：

```cpp
case RiscvToy::PseudoCALLIndirect:
  expandPseudoCALLIndirect(MI, OS, Fixups, STI);
  return;
```

展开函数：

```cpp
MCInst JALR;
JALR.setOpcode(RiscvToy::RiscvToyJALR);
JALR.addOperand(MCOperand::createReg(RiscvToy::X1));
JALR.addOperand(MI.getOperand(0));  // 函数地址所在寄存器
JALR.addOperand(MCOperand::createImm(0));
emitRawBinary(getBinaryCodeForInstr(JALR, Fixups, STI), OS);
```

结果就是：

```text
jalr x1, 0(fnreg)
```

## 6. 实测输出

输入：

```llvm
define i32 @indirect(i32 (i32)* %f, i32 %x) {
  %r = call i32 %f(i32 %x)
  ret i32 %r
}
```

输出：

```asm
addi sp, sp, -16
sw ra, 12(sp)
addi a2, a0, 0      # 函数指针放到 a2
addi a0, a1, 0      # 实参放到 a0
jalr ra, 0(a2)      # 间接调用
lw ra, 12(sp)
addi sp, sp, 16
ret
```

object 反汇编对应：

```asm
jalr ra, 0(a2)
```

没有 `R_RISCV_JAL` relocation，因为目标地址本来就在寄存器中。

## 7. 测试

新增：

```text
backend/test/CodeGen/RiscvToy/19-indirect-call.ll
```

测试同时跑 `-filetype=asm` 和 `-filetype=obj`，确认：

1. 文本输出有 `jalr ra, 0(a2)`；
2. object 也能成功生成；
3. `ra` 的保存/恢复和直接调用一致。

运行：

```bash
./scripts/test-cpu0.sh
```

当前结果：161 个 Cpu0/RiscvToy 测试全部通过。

## 8. 本阶段边界

Stage 13 已完成：

- 函数指针/间接 callee 不再报不支持；
- 间接调用走 `jalr x1, 0(rs1)`；
- caller-saved 信息与直接调用一致；
- assembly 和 object 两种输出都可用。

尚未支持：

- 尾调用/尾跳转；
- 全局数据地址（`la symbol`）；
- 栈上传参；
- varargs。
