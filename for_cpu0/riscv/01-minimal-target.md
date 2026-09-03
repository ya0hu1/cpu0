# RiscvToy Stage 1：最小 target 注册

## 1. 本阶段目标

这一阶段还不需要生成 RISC-V 指令。目标只做一个可以被 LLVM 认识的
`RiscvToy` target：

```text
target 名称: riscvtoy
架构:        Triple::riscv32
数据布局:    e-m:e-p:32:32-i64:64-n32-S128
可生成代码:  暂时不支持
```

验证方式：

```bash
build/bin/llc --version
```

输出中应该看到：

```text
riscvtoy - Educational RV32 backend
```

如果直接把 LLVM IR 交给 `riscvtoy`，会得到明确错误：

```text
llc: error: target does not support generation of this file type
```

这是有意为之，避免一个没有寄存器、没有指令表的 target 在 CodeGen 中崩溃。

## 2. 目录结构

```text
backend/RiscvToy/
  CMakeLists.txt
  RiscvToyTargetMachine.h
  RiscvToyTargetMachine.cpp
  TargetInfo/
    CMakeLists.txt
    RiscvToyTargetInfo.h
    RiscvToyTargetInfo.cpp
backend/test/CodeGen/RiscvToy/
  01-target-registration.ll
```

对照 Cpu0：

```text
Cpu0 是完整后端，包含 CodeGen、MCTargetDesc、InstPrinter、AsmParser、Disassembler
RiscvToy 是最小骨架，先只解决“如何被 LLVM 认识”
```

## 3. 文件作用

### 3.1 `CMakeLists.txt`

```cmake
add_llvm_component_group(RiscvToy)

add_llvm_target(RiscvToyCodeGen
  RiscvToyTargetMachine.cpp
  ...
  ADD_TO_COMPONENT
  RiscvToy
  )

add_subdirectory(TargetInfo)
```

LLVM 的构建系统会为每个 target 生成 `LLVMRiscvToyInfo` 和
`LLVMRiscvToyCodeGen` 这类组件库。`llc` 通过：

```text
AllTargetsInfos
AllTargetsCodeGens
```

把这些组件链接进来。

如果只注册 TargetInfo，不创建 `LLVMRiscvToyCodeGen`，LLVM 的
`LLVM-Config.cmake` 会报：

```text
Target RiscvToy is not in the set of libraries.
```

所以这里放了一个最小的 codegen 库。

### 3.2 `TargetInfo/RiscvToyTargetInfo.cpp`

```cpp
Target &llvm::getTheRiscvToyTarget() {
  static Target TheRiscvToyTarget;
  return TheRiscvToyTarget;
}

extern "C" void LLVMInitializeRiscvToyTargetInfo() {
  RegisterTarget<Triple::riscv32> X(
      getTheRiscvToyTarget(), "riscvtoy", "Educational RV32 backend",
      "RiscvToy");
}
```

这里有两个关键点：

1. `RegisterTarget` 把静态的 `Target` 对象放进 `TargetRegistry`；
2. 架构类型直接复用 `Triple::riscv32`，而不是发明新的 `Triple::riscvtoy`。

复用 `riscv32` 是因为 RISC-V 的 Triple 支持已经很完整。RiscvToy 是教学后端，
重点放在指令和寄存器模型，不需要先把 Triple 架构表复制一遍。

### 3.3 Triple 别名

`llvm-overlay/llvm/lib/Support/Triple.cpp` 增加：

```cpp
.Case("riscvtoy", Triple::riscv32)
```

所以：

```bash
llc -mtriple=riscvtoy-unknown-unknown
```

可以把 `riscvtoy` 解析成 32 位 RISC-V 架构，再通过 target 名 `riscvtoy`
找到 RiscvToy。

### 3.4 `RiscvToyTargetMachine.cpp`

```cpp
RiscvToyTargetMachine::RiscvToyTargetMachine(...)
    : LLVMTargetMachine(T, "e-m:e-p:32:32-i64:64-n32-S128", TT, CPU, FS,
                        Options, getEffectiveRelocModel(RM),
                        CM.hasValue() ? *CM : CodeModel::Small, OL) {}

bool RiscvToyTargetMachine::addPassesToEmitFile(...) {
  return true;
}
```

要点：

- 继承 `LLVMTargetMachine`，让 `llc` 的 `static_cast` 和 DataLayout 都合法；
- 提供 RV32 小端 DataLayout；
- 重写 `addPassesToEmitFile()` 直接返回 `true`；
- LLVM 会把 `true` 翻译成“不支持生成这种文件类型”，而不是进入不完整的 CodeGen。

等到后面阶段补上寄存器、指令和 MC 层后，再删除这个“不支持”重写。

## 4. 为什么不在 Stage 1 复制上游 RISCV

上游 `lib/Target/RISCV` 已经包含：

- RV32/RV64；
- M/A/F/D/C/V 扩展；
- TLS、relaxation、GlobalISel；
- 完整 AsmParser/Disassembler/MCTargetDesc。

直接复制会得到：

1. 大量一时读不完的 TableGen；
2. 难以在每一步只验证一个概念；
3. 与“从 Cpu0 理解最小后端”的学习路径脱节。

RiscvToy 保留命名空间，不干扰上游 `RISCV` target。

## 5. 下一步

Stage 2 将定义 RV32I 通用寄存器：

```text
zero, ra, sp, gp, tp, t0-t6, s0-s11, a0-a7
```

重点回答：

- 为什么 `zero` 不参与分配；
- 为什么 `sp` 单独处理；
- caller-saved 与 callee-saved 如何划分。
