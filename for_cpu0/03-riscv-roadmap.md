# 从 Cpu0 到 RISC-V 后端的路线

本仓库先保证 LBD 的 Cpu0 后端可以构建和输出汇编，再在这个工作区中逐步实现一个教学型 RISC-V 后端。这里的 RISC-V 后端不是替代上游 LLVM 的 RISCV target，而是为了练习后端结构。

## 总体原则

1. 先跑通一个最小 target；
2. 每个阶段只加一类能力；
3. 每个阶段都配一个 `.ll` 测试和中文说明；
4. 不要一开始复制完整 RISCV target。

## 目录规划

```text
backend/RiscvToy/
  RiscvToy.td
  RiscvToyRegisterInfo.td
  RiscvToyInstrInfo.td
  RiscvToyInstrFormats.td
  RiscvToyCallingConv.td
  RiscvToyTargetMachine.cpp
  TargetInfo/
  AsmParser/
  MCTargetDesc/
  Disassembler/
for_cpu0/
  riscv/
    01-minimal-target.md
    02-registers-and-callingconv.md
    03-instruction-encoding.md
    04-first-codegen.md
    05-function-call-frame.md
    06-branches-select.md
    07-mc-object.md
    08-disassembler.md
    09-asm-parser.md
    10-large-constants.md
    11-shifts.md
backend/test/CodeGen/RiscvToy/
```

目录名先用 `RiscvToy`，避免和 LLVM 上游 `RISCV` 目标名冲突。等稳定后再讨论是否改名。

## 阶段 0：Cpu0 基线

目标：

- LLVM 12 源码接入；
- `llc` 能识别 `cpu0`；
- 能把一个简单 `add` 函数打印成 Cpu0 汇编；
- 至少跑通少量 CodeGen 测试。

已完成，验证结果见
[05-cpu0-baseline-result.md](05-cpu0-baseline-result.md)。

## 阶段 1：最小 RISC-V target 注册

已完成 Stage 1 代码，说明见
[riscv/01-minimal-target.md](riscv/01-minimal-target.md)。
它先不做指令选择，只让 LLVM 知道：

```text
Triple: riscvtoy
Target: RiscvToy
DataLayout: e-m:e-p:32:32-i64:64-n32-S128
```

实现说明：

- RiscvToy 直接复用 LLVM 已有的 `Triple::riscv32`，所以不需要新增架构枚举；
- `Triple.cpp` 增加 `riscvtoy -> riscv32` 别名；
- 当前 `addPassesToEmitFile()` 返回不支持，避免无 CodeGen 时崩溃。

产出：

```text
RiscvToyTargetMachine.{cpp,h}
TargetInfo/RiscvToyTargetInfo.{cpp,h}
最小 CMakeLists.txt
```

验证：

```bash
build/bin/llc --version
```

`--version` 会列出 `riscvtoy`。尝试让 `riscvtoy` 生成汇编时会得到
“不支持生成这种文件类型”的明确错误。

## 阶段 2：寄存器模型

已完成寄存器表和调用约定表，说明见
[riscv/02-registers-and-callingconv.md](riscv/02-registers-and-callingconv.md)。

定义 RV32I 常用整数寄存器：

```text
zero, ra, sp, gp, tp, t0-t6,
s0-s11, a0-a7
```

TableGen 中先定义：

```tablegen
class RiscvToyReg<bits<5> num, string n> : Register<n> {
  let HWEncoding = num;
}
```

然后定义寄存器类：

```tablegen
def GPR : RegisterClass<"RiscvToy", [i32], 32,
  (add X1, X2, X3, X4, X5, X6, X7)> {
  let AllocationPriority = 0;
}
```

本阶段回答：

- 哪些寄存器可用于分配？
- `zero` 为什么不参与分配？
- `sp` 为什么通常单独处理？
- caller-saved 和 callee-saved 如何划分？

## 阶段 3：指令编码模型

已完成 RV32I 指令表和 SelectionDAG pattern，说明见
[riscv/03-instruction-encoding.md](riscv/03-instruction-encoding.md)。

本阶段加入了：

- R 型指令：`add`、`sub`、`and`、`or`、`xor`；
- I 型指令：`addi`；
- `jalr` 真指令和 `ret` pseudo；
- `PseudoRET` 到 `jalr x0, ra, 0` 的展开关系；
- `GPRFull`，让机器指令可以显式引用 `x0`；
- `RiscvToyGenInstrInfo.inc` 与 `RiscvToyGenDAGISel.inc`。

## 阶段 4：首个 RISC-V 汇编输出

已完成。说明见
[riscv/04-first-codegen.md](riscv/04-first-codegen.md)。

这个里程碑同时接入了 SelectionDAG 和 MC 打印层。现在 `llc` 可以把：

```llvm
define i32 @add(i32 %a, i32 %b) {
  %sum = add i32 %a, %b
  ret i32 %sum
}
```

打印成：

```asm
add:
  add a0, a0, a1
  ret
```

加入的标准对象：

```text
RiscvToySubtarget
RiscvToyRegisterInfo
RiscvToyInstrInfo
RiscvToyTargetLowering
RiscvToyFrameLowering
RiscvToyAsmPrinter
MCTargetDesc/
```

调用约定当前只支持：

- 前 8 个整数参数放 `a0-a7`；
- 返回值放 `a0`；
- 16 字节栈对齐；
- 无函数调用、无栈上局部变量。

## 阶段 5：函数调用和栈帧

引入函数调用后必须处理：

- `ra` 的保存和恢复；
- `sp` 调整；
- 局部变量；
- 溢出；
- 16 字节栈对齐。

已完成。中文说明见
[riscv/05-function-call-frame.md](riscv/05-function-call-frame.md)。

目前支持：

- `lw/sw`；
- `alloca` 与 FrameIndex 消除；
- callee-saved 保存恢复，包括 `ra`；
- 直接函数调用和 8 个以内的寄存器参数；
- 分段的大栈帧调整。

尚未支持：栈上传参、varargs、间接调用、复杂返回值和 object 文件编码。
后两类留给 MC 层阶段处理。

## 阶段 6：分支、比较和条件选择

已完成。中文说明见
[riscv/06-branches-select.md](riscv/06-branches-select.md)。

本阶段加入了：

- B 型条件分支；
- `slt/sltu/slti/sltiu` 与 `xori`；
- `setcc` 到 0/1 的 pattern；
- `br` 与 `PseudoBR`；
- `select` 通过 custom inserter 展开成分支和 PHI；
- 小整数常量；
- 基础 `analyzeBranch/insertBranch/removeBranch`。

尚未支持大常量、switch、间接跳转和分支 fixup。这些和真实机器码编码一起进入
Stage 7。

## 阶段 7：完整 MC 层

已完成。说明见
[riscv/07-mc-object.md](riscv/07-mc-object.md)。

按 Cpu0 的结构补的是：

```text
MCTargetDesc/
  MCCodeEmitter
  AsmBackend
  ELFObjectWriter
```

现在：

1. `llc -filetype=obj` 输出 ELF object；
2. `PseudoBR/PseudoCALL/PseudoRET` 在编码时展开为真实 `jal/jalr`；
3. 本地分支由 AsmBackend fixup 填值；
4. 外部调用输出 `R_RISCV_JAL` relocation。

说明：Stage 4 已经完成了 `InstPrinter` 和文本汇编输出，这一阶段主要让
`-filetype=obj` 和后续工具链真正可执行。

## 阶段 8：Disassembler

已完成反汇编器，说明见
[riscv/08-disassembler.md](riscv/08-disassembler.md)。

现在：

1. `llvm-objdump -d` 能反汇编 RiscvToy `.o`；
2. 手写 decoder 按 opcode/funct3/funct7 解码 RV32I 子集；
3. B/J 型立即数按 RISC-V 散位格式恢复；
4. `ret` 别名在反汇编中保留。

## 阶段 9：AsmParser

已完成汇编器，说明见
[riscv/09-asm-parser.md](riscv/09-asm-parser.md)。

现在：

1. `llvm-mc` 能把 RiscvToy 汇编文本解析成 MCInst/object；
2. 寄存器 ABI 名与 `x0-x31` 都可识别；
3. `offset(base)` memory 语法可用；
4. `ret/call/j` pseudo 与 CodeGen/MC 层共用同一套语法；
5. `llc -filetype=asm` 的输出可以重新汇编成 `.o`。

## 阶段 10：32 位常量 materialization

已完成，说明见
[riscv/10-large-constants.md](riscv/10-large-constants.md)。

现在：

1. 超出 12 位的常量用 `lui + addi` 合成；
2. 小常量仍走单条 `addi x0`；
3. `lui` 已同步支持 object 编码、AsmParser 和 Disassembler；
4. 正数、负数和 `-4096` 边界都有 lit 测试。

## 阶段 11：移位指令

已完成，说明见
[riscv/11-shifts.md](riscv/11-shifts.md)。

现在：

1. `shl/lshr/ashr` 支持立即数移位；
2. `shl/lshr/ashr` 支持变量移位；
3. `slli/srli/srai` 的特殊 funct7/shamt 编码可用；
4. parser、object、disassembler 都覆盖 6 条移位指令。

后续候选：

1. 全局寻址：加入 `auipc`、`%pcrel_hi/%pcrel_lo`、`la` 等；
2. 补齐常用 RV32I 指令，如 `lb/lh/lbu/lhu/sb/sh`；
3. 分支范围外处理：为超过 B/J 型范围的跳转生成合法长序列。

每个阶段仍然先加一个能被 lit 自动回归的 `.ll`/`.s` 测试，再做提交。

## 为什么不从复制上游 RISCV 开始

上游 RISCV target 很完整，包含：

- RV32/RV64；
- M/A/F/D/C 扩展；
- vector；
- ABI、TLS、relaxation；
- 大量优化。

直接复制会让初学者看不出最小骨架。正确的学习顺序是：用 Cpu0 理解后端全貌，再用最小组件拼 RISC-V。
