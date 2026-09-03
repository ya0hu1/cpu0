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
  RiscvToyCallingConv.td
  RiscvToyTargetMachine.cpp
  TargetInfo/
  MCTargetDesc/
  InstPrinter/
for_cpu0/
  riscv/
    01-minimal-target.md
    02-registers-and-callingconv.md
backend/test/CodeGen/RiscvToy/
```

目录名先用 `RiscvToy`，避免和 LLVM 上游 `RISCV` 目标名冲突。等稳定后再讨论是否改名。

## 阶段 0：Cpu0 基线

目标：

- LLVM 12 源码接入；
- `llc` 能识别 `cpu0`；
- 能把一个简单 `add` 函数打印成 Cpu0 汇编；
- 至少跑通少量 CodeGen 测试。

这一阶段还没有完成，因为当前环境需要网络拉取 LLVM 12。

## 阶段 1：最小 RISC-V target 注册

先不做指令选择，只让 LLVM 知道：

```text
Triple: riscvtoy
Target: RiscvToy
DataLayout: E-m:e-p:32:32-i64:64-n32-S64
```

说明：

- 这里先用大端例子；实际 RV32I 通常是小端，所以后面会改成 `e`；
- 需要新增 `Triple::riscvtoy` 时，不能直接覆盖上游 Triple 文件，后续可以迁移到 LLVM 的 target registration 或独立示例工具。

产出：

```text
RiscvToyTargetMachine.{cpp,h}
TargetInfo/RiscvToyTargetInfo.{cpp,h}
最小 CMakeLists.txt
```

验证：

```bash
build/bin/llc -mtriple=riscvtoy --version
```

## 阶段 2：寄存器模型

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

## 阶段 3：最小指令和调用约定

先支持这些 LLVM IR 场景：

```llvm
define i32 @add(i32 %a, i32 %b) {
  %sum = add i32 %a, %b
  ret i32 %sum
}
```

需要定义：

- `ADD`、`SUB`、`AND`、`OR`、`XOR`；
- `ADDI`；
- `LUI`；
- `JAL`、`JALR`；
- load/store 指令。

调用约定先只处理：

- 第一个整数参数放 `a0`；
- 第二个整数参数放 `a1`；
- 返回值放 `a0`；
- 栈对齐 16 字节。

这个阶段的核心是让 `llc` 输出：

```asm
add:
  add a0, a0, a1
  ret
```

实际 pseudoinstruction expansion 中，`ret` 通常由 `JALR x0, ra, 0` 展开。

## 阶段 4：栈帧和 callee-saved 寄存器

引入函数调用后必须处理：

- `ra` 的保存和恢复；
- `sp` 调整；
- 局部变量；
- 溢出；
- 16 字节栈对齐。

对应文件：

```text
RiscvToyFrameLowering.cpp
RiscvToyInstrInfo.cpp
RiscvToyRegisterInfo.cpp
```

## 阶段 5：分支、比较和条件选择

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

## 阶段 6：MC 层

按 Cpu0 的结构补：

```text
InstPrinter/
MCTargetDesc/
AsmParser/（可选后置）
Disassembler/（可选后置）
```

优先级：

1. InstPrinter 能打印汇编；
2. MCCodeEmitter 能编码 32 位指令；
3. AsmBackend 支持 PC-relative fixup；
4. ELFObjectWriter 输出目标文件。

## 阶段 7：测试体系

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
