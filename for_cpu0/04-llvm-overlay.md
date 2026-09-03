# llvm-overlay 文件解析

## 1. 为什么需要 overlay

LLVM 的 target 不只是 `llvm/lib/Target/Cpu0` 这一棵树。要让 LLVM 的公共代码认识 Cpu0，还需要修改一些和 target 无关的公共文件。本仓库不能提交完整的 LLVM 源码，所以把这批修改集中在：

```text
llvm-overlay/llvm/
```

`scripts/setup-llvm.sh` 会先把这批文件复制到 `third_party/llvm-project/llvm/` 下，再把 `backend/Cpu0/` 复制到 `llvm/lib/Target/Cpu0/`。

简单记忆：

```text
backend/Cpu0/     是新增的 Cpu0 后端主体
llvm-overlay/llvm/ 是让 LLVM 公共代码认识 Cpu0 的补丁
```

## 2. 文件分组

### 2.1 让 Triple 认识 cpu0 / cpu0el

```text
include/llvm/ADT/Triple.h
lib/Support/Triple.cpp
```

`Triple` 是 LLVM 解析 `-mtriple=...` 的目标描述类。它把字符串架构名映射成枚举。

这两个文件新增了两个架构值：

```cpp
cpu0,     // CPU0, big endian
cpu0el,   // CPU0, little endian
```

否则 `llc -mtriple=cpu0` 会把 `cpu0` 当作未知架构，无法进入 Cpu0 后端。

### 2.2 让 ELF 层认识 Cpu0 的重定位

```text
include/llvm/BinaryFormat/ELF.h
include/llvm/BinaryFormat/ELFRelocs/Cpu0.def
```

LLVM 生成 ELF 目标文件时，需要知道机器号和重定位类型。

`ELF.h` 增加 `EM_CPU0` 之类的机器号。`Cpu0.def` 定义 Cpu0 自己的重定位：

```cpp
ELF_RELOC(R_CPU0_HI16,  5)
ELF_RELOC(R_CPU0_LO16,  6)
ELF_RELOC(R_CPU0_GOT16, 9)
...
```

第一次只生成汇编时不一定触发这些代码，但一旦进入 `-filetype=obj`、汇编器或 objdump 阶段，它们就是必须的。

### 2.3 让 Cpu0 有自己的 intrinsic

```text
include/llvm/IR/Intrinsics.td
include/llvm/IR/IntrinsicsCpu0.td
```

LLVM intrinsic 是一种带目标语义的内建函数。LBD 用 Cpu0 的 gcd 做示例：

```tablegen
def int_cpu0_gcd : GCCBuiltin<"__builtin_cpu0_gcd">, ...
```

`Intrinsics.td` 在最后包含 `IntrinsicsCpu0.td`。这样构建 `intrinsics_gen` 时，Cpu0 特有 intrinsic 也会生成到 `.inc` 文件里。

### 2.4 让 ELFObjectFile 能读取 Cpu0 对象

```text
include/llvm/Object/ELFObjectFile.h
lib/Object/ELF.cpp
```

`llvm-objdump` 和 `llvm-readobj` 通过 `ELFObjectFile` 读 ELF。修改内容包括：

- CPU0 的机器号识别；
- CPU0 relocation 的读取；
- objdump 打印时能调用 Cpu0 的指令反汇编逻辑。

当前仓库后端中的测试已经包含 `llvm-objdump -d` 的用例，因此这组文件不能省略。

### 2.5 让 MCSubtargetInfo 正确打印 Cpu0

```text
lib/MC/MCSubtargetInfo.cpp
```

MC 层在输出带 CPU 名称的信息时，会查这张表。这里加入 `cpu032I`、`cpu032II`，让 `-mcpu=cpu032II` 有对应名称。

### 2.6 让 LLVM 构建系统包含 Cpu0

```text
CMakeLists.txt
cmake/config-ix.cmake
include/module.modulemap
```

顶层 `CMakeLists.txt` 和 `config-ix.cmake` 负责把 `Cpu0` 放进可配置的 target 列表。`module.modulemap` 只是保证模块构建下也能找到头文件。

### 2.7 工具与辅助修改

```text
tools/llvm-objdump/llvm-objdump.cpp
tools/elf2hex/*
utils/gn/secondary/llvm/lib/Target/targets.gni
```

- `llvm-objdump.cpp`：让 objdump 能列出 Cpu0 的架构并调用 Cpu0 反汇编；
- `elf2hex`：LBD 教程用于把 ELF 转成 Verilog 初始化 hex，属于可选的扩展工具；
- `targets.gni`：GN 构建系统中的 target 列表，影响不使用 CMake 的构建方式。

这些文件不会进入最终 Cpu0 后端源码，但保持齐全可以避免“换一种构建方式就缺文件”的问题。

## 3. 之前的缺口

仓库最初的 `llvm-overlay` 只复制了 7 个文件：

```text
CMakeLists.txt
cmake/config-ix.cmake
include/module.modulemap
lib/MC/MCSubtargetInfo.cpp
lib/Object/ELF.cpp
lib/Support/Triple.cpp
tools/llvm-objdump/llvm-objdump.cpp
```

对照 LBD 当前提交 `8e5c43ff1b21cd8dd4eb03d6d2ae5099b4eadd64` 后，补齐了：

```text
include/llvm/ADT/Triple.h
include/llvm/BinaryFormat/ELF.h
include/llvm/BinaryFormat/ELFRelocs/Cpu0.def
include/llvm/IR/Intrinsics.td
include/llvm/IR/IntrinsicsCpu0.td
include/llvm/Object/ELFObjectFile.h
tools/elf2hex/*
utils/gn/secondary/llvm/lib/Target/targets.gni
```

`backend/Cpu0/` 本身与 LBD 源码逐文件一致，所以这次主要补的是公共层文件。

## 4. 验证方式

运行 setup 后可以检查：

```bash
test -f third_party/llvm-project/llvm/include/llvm/BinaryFormat/ELFRelocs/Cpu0.def
test -f third_party/llvm-project/llvm/lib/Target/Cpu0/CMakeLists.txt
test -f third_party/llvm-project/llvm/test/CodeGen/Cpu0/lit.local.cfg
```

如果后面新增了 RISC-V 教学 target，同样需要回答三个问题：

1. Triple 是否认识新的架构名？
2. ELF 是否知道新的机器号和重定位？
3. CMake 是否把新 target 目录加入构建？

这三个问题比直接抄一份 RISCV target 更值得先弄明白。
