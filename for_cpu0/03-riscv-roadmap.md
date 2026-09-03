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
  MCTargetDesc/
  InstPrinter/
for_cpu0/
  riscv/
    01-minimal-target.md
    02-registers-and-callingconv.md
    03-instruction-encoding.md
    04-first-codegen.md
    05-function-call-frame.md
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

LLVM IR 中的：

```llvm
icmp
br
select
```

会下降成 DAG 比较节点。RV32I 没有复杂条件码，所以通常要映射到：

```text
slti / sltiu / slt / sltu
bne / beq / blt / bge / bltu / bgeu
```

这个阶段最容易暴露 legalizer 和 pattern 的理解问题。

## 阶段 7：完整 MC 层

按 Cpu0 的结构补：

```text
InstPrinter/
MCTargetDesc/
AsmParser/（可选后置）
Disassembler/（可选后置）
```

优先级：

1. MCCodeEmitter 能编码 32 位指令；
2. AsmBackend 支持 PC-relative fixup；
3. ELFObjectWriter 输出目标文件。

说明：Stage 4 已经完成了 `InstPrinter` 和文本汇编输出，这一阶段主要让
`-filetype=obj` 和后续工具链真正可执行。

## 阶段 8：测试体系

每个阶段至少加一个 LLVM lit 测试：

```llvm
; RUN: llc -mtriple=riscvtoy < %s | FileCheck %s

define i32 @add(i32 %a, i32 %b) {
; CHECK-LABEL: add:
; CHECK: add a0, a0, a1
; CHECK: ret
  %sum = add i32 %a, %b
  ret i32 %sum
}
```

测试名和阶段对应，例如：

```text
01-add.ll
02-callingconv.ll
03-frame.ll
04-branch.ll
```

## 为什么不从复制上游 RISCV 开始

上游 RISCV target 很完整，包含：

- RV32/RV64；
- M/A/F/D/C 扩展；
- vector；
- ABI、TLS、relaxation；
- 大量优化。

直接复制会让初学者看不出最小骨架。正确的学习顺序是：用 Cpu0 理解后端全貌，再用最小组件拼 RISC-V。
