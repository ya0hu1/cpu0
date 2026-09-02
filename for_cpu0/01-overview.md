# LLVM Cpu0 后端总览

这份笔记解释 Jonathan2251/lbd 提供的 Cpu0 后端为什么需要这些目录和文件，以及 LLVM 从 LLVM IR 到机器码的主干流程。

## 1. LLVM 后端在编译器中的位置

一个简化后的编译过程是：

```text
C/C++ 源码
  ↓ Clang 前端
LLVM IR
  ↓ LLVM 中间优化
目标无关优化后的 LLVM IR
  ↓ SelectionDAG / 指令选择
目标机器指令
  ↓ 寄存器分配
物理寄存器上的机器指令
  ↓ 汇编输出 / 目标文件输出
.s / .o / ELF
```

LLVM 后端的核心任务不是理解 C++，而是把目标无关的 LLVM IR 变成某个具体 CPU 的机器码。Cpu0 是一个教学用 32 位 CPU，LBD 用它把后端中的关键组件逐步展示出来。

## 2. Cpu0 后端的核心组成

### 2.1 `Cpu0TargetMachine`

文件：

```text
backend/Cpu0/Cpu0TargetMachine.cpp
backend/Cpu0/Cpu0TargetMachine.h
```

这是目标机器的入口之一。它负责：

- 向 LLVM 注册大端目标 `Cpu0` 和小端目标 `Cpu0el`；
- 描述 Cpu0 的 DataLayout，例如 32 位指针、栈对齐；
- 创建 `TargetPassConfig`，把指令选择等后端 pass 接入流水线；
- 根据函数属性创建对应的 `Cpu0Subtarget`。

### 2.2 TableGen 描述文件

LLVM 后端大量使用 TableGen。Cpu0 中的主要 `.td` 文件如下：

| 文件 | 作用 |
|---|---|
| `Cpu0.td` | 目标顶层描述，组合子目标、指令集和汇编 parser |
| `Cpu0RegisterInfo.td` | 寄存器和寄存器类 |
| `Cpu0InstrInfo.td` | 指令模式和指令编码 |
| `Cpu0InstrFormats.td` | 指令格式 |
| `Cpu0CallingConv.td` | 调用约定 |
| `Cpu0Schedule.td` | 简单调度信息 |
| `Cpu0Asm.td` | 汇编 parser 相关 TableGen 定义 |
| `Cpu0Other.td` | 目标级杂项定义 |

构建时，CMake 会调用 TableGen 生成类似下面的文件：

```text
Cpu0GenRegisterInfo.inc
Cpu0GenInstrInfo.inc
Cpu0GenDAGISel.inc
Cpu0GenCallingConv.inc
Cpu0GenAsmWriter.inc
Cpu0GenSubtargetInfo.inc
```

这些 `.inc` 文件不是手写的，而是从 `.td` 文件生成的。

### 2.3 指令选择

关键文件：

```text
Cpu0ISelDAGToDAG.cpp
Cpu0ISelDAGToDAG.h
Cpu0SEISelDAGToDAG.cpp
Cpu0SEISelDAGToDAG.h
Cpu0ISelLowering.cpp
Cpu0SEISelLowering.cpp
```

SelectionDAG 指令选择的大致过程：

1. LLVM IR 先被 lowering 成 SelectionDAG 节点；
2. `Cpu0ISelLowering` 把不合法或不适合 Cpu0 的节点改写成合法节点；
3. TableGen 中的 pattern 生成 `Cpu0GenDAGISel.inc`；
4. `Cpu0SEISelDAGToDAG` 执行模式匹配，把 DAG 节点选成 Cpu0 机器指令。

可以把这一步理解成：LLVM IR 的抽象运算，逐渐变成 Cpu0 能识别的具体指令。

### 2.4 寄存器与调用约定

关键文件：

```text
Cpu0RegisterInfo.td
Cpu0RegisterInfo.cpp
Cpu0RegisterInfo.h
Cpu0CallingConv.td
```

这部分回答几个问题：

- Cpu0 有哪些通用寄存器？
- 哪些寄存器是 caller-saved，哪些是 callee-saved？
- 函数参数放在哪里？
- 返回值放在哪里？
- 栈指针和帧指针如何维护？

### 2.5 栈帧管理

关键文件：

```text
Cpu0FrameLowering.cpp
Cpu0SEFrameLowering.cpp
```

栈帧管理负责函数入口和出口：

- 函数入口保存必要的 callee-saved 寄存器；
- 调整栈指针；
- 函数出口恢复寄存器并返回；
- 为局部变量、溢出值、 outgoing call arguments 分配栈空间。

### 2.6 MC 层

目录：

```text
backend/Cpu0/MCTargetDesc/
backend/Cpu0/InstPrinter/
backend/Cpu0/AsmParser/
backend/Cpu0/Disassembler/
```

MC 层处理更靠近机器码的部分：

- `InstPrinter`：把 `MCInst` 打印成汇编文本；
- `AsmParser`：把汇编文本解析成 `MCInst`；
- `MCCodeEmitter`：把 `MCInst` 编码成二进制指令；
- `AsmBackend`：处理 relocation、 relaxation 等；
- `ELFObjectWriter`：生成 ELF 目标文件。

### 2.7 目标注册

关键文件：

```text
backend/Cpu0/TargetInfo/Cpu0TargetInfo.cpp
backend/Cpu0/MCTargetDesc/Cpu0MCTargetDesc.cpp
```

`LLVMInitializeCpu0TargetInfo()` 注册 `cpu0` 和 `cpu0el` 的 TargetInfo。

`LLVMInitializeCpu0Target()` 注册 `Cpu0TargetMachine`。

LLVM 工具 `llc` 初始化所有目标时，就会调用这些函数。

## 3. `llvm-overlay` 的作用

LBD 不是完全插件式地把 Cpu0 接入 LLVM。它还需要修改少量 LLVM 上游文件：

```text
llvm/CMakeLists.txt
llvm/cmake/config-ix.cmake
llvm/include/llvm/module.modulemap
llvm/lib/MC/MCSubtargetInfo.cpp
llvm/lib/Object/ELF.cpp
llvm/lib/Support/Triple.cpp
llvm/tools/llvm-objdump/llvm-objdump.cpp
```

这些修改主要解决：

- 让 LLVM 构建系统知道 `Cpu0` 是一个 target；
- 让 Triple 解析 `cpu0`、`cpu0el`；
- 让 `llvm-objdump` 识别 Cpu0；
- 处理 Cpu0 相关 ELF/MC 行为。

本仓库把这些文件放在 `llvm-overlay/`，由 `scripts/setup-llvm.sh` 覆盖到 LLVM 12 源码树。

## 4. `Cpu0SetChapter.h` 和 LBD 教学结构

LBD 源码中有类似：

```cpp
#if CH >= CH3_1
#endif
```

这便于逐章展开后端实现。当前 `Cpu0SetChapter.h` 中是：

```cpp
#define CH CH12_1
```

表示使用教程后期的完整实现。阅读源码时，可以暂时把它改小，例如：

```cpp
#define CH CH3_1
```

然后重新构建，观察最小后端需要哪些能力。

## 5. 为什么要固定 LLVM 12 提交

LBD 的构建脚本使用 LLVM 12 分支中的固定提交：

```text
e8a397203c67adbeae04763ce25c6a5ae76af52c
```

LLVM API 经常变化。例如现代 LLVM 中 `Optional` 被逐渐替换为 `std::optional`，目标注册接口也可能变化。因此本仓库的 Cpu0 后端应配合这个固定版本构建。

等 Cpu0 基线跑通后，再考虑迁移到更新版本。
