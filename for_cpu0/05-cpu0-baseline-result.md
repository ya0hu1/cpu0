# Cpu0 基线构建与验证结果

## 1. 本步骤做了什么

这一步的目标是把 LBD 的 Cpu0 后端真正“跑起来”，而不是只放源码。

实际完成的内容：

1. 拉取固定 LLVM 12 源码：

```text
e8a397203c67adbeae04763ce25c6a5ae76af52c
```

2. 对照 LBD 提交：

```text
8e5c43ff1b21cd8dd4eb03d6d2ae5099b4eadd64
```

3. 补齐 `llvm-overlay` 中缺失的 Cpu0 公共层文件；
4. 应用 overlay，把 `backend/Cpu0` 复制进 LLVM；
5. 构建 `llc`、`opt`、`llvm-as`、`llvm-dis`、`clang` 和
   `llvm-objdump`；
6. 验证汇编输出、C 前端路径、ELF 对象输出、objdump 反汇编和 lit 回归测试。

## 2. 本机环境

这里构建用的环境：

- Ubuntu 22.04；
- CMake 3.22；
- GNU Make 代替 Ninja；
- `JOBS=4` 控制并行度；
- Release 构建；
- 只选择 `LLVM_TARGETS_TO_BUILD=Cpu0`。

`scripts/build-cpu0.sh` 会自动检测 `ninja`，没有时回退到 Makefiles。

## 3. 关键验证命令

### 3.1 `llc` 能识别 Cpu0

```bash
build/bin/llc --version
```

关键输出：

```text
Registered Targets:
  cpu0   - CPU0 (32-bit big endian)
  cpu0el - CPU0 (32-bit little endian)
```

### 3.2 最小加法

```bash
build/bin/llc -march=cpu0 examples/cpu0/add.ll -o -
```

函数主体输出：

```asm
add:
	addu	$2, $4, $5
	ret	$lr
```

对应关系：

- `$4`：第一个 i32 参数；
- `$5`：第二个 i32 参数；
- `$2`：i32 返回值；
- `$lr`：返回地址寄存器。

### 3.3 C 到 LLVM IR 到 Cpu0 汇编

LBD 教程不要求 Clang 维护完整的 CPU0 驱动，而是让 Clang 先按 MIPS triple
生成 IR：

```bash
build/bin/clang -target mips-unknown-linux-gnu -S -emit-llvm \
  examples/cpu0/add.c -o /tmp/add.ll
build/bin/llc -march=cpu0 /tmp/add.ll -o /tmp/add.s
```

### 3.4 目标文件与反汇编

```bash
build/bin/llc -march=cpu0 -mcpu=cpu032II \
  -relocation-model=pic -filetype=obj \
  backend/test/CodeGen/Cpu0/addi.ll -o /tmp/addi.o
build/bin/llvm-objdump -d /tmp/addi.o
```

第一行会显示：

```text
/tmp/addi.o:	file format elf32-cpu0
```

这说明 ELF overlay 也生效了。

### 3.5 回归测试

```bash
./scripts/test-cpu0.sh
```

实际结果：

```text
-- Testing: 137 tests, 12 workers --
Passed: 137
```

`test-cpu0.sh` 直接调用 `build/bin/llvm-lit`，不会触发整个 LLVM 的
`check-all` 构建，适合在后端开发时快速验证。

## 4. 这次发现并修掉的问题

仓库最初的 overlay 只覆盖 7 个文件。缺少的公共层文件会导致以下风险：

- CPU0 架构只写在 `Triple.cpp`，没有同步写进 `Triple.h`；
- ELF 机器号和 Cpu0 relocation 定义不全；
- `IntrinsicsCpu0.td` 没有被包含；
- `ELFObjectFile` 不能完整识别 Cpu0 对象；
- GN/elf2hex 等工具配置缺失。

现在的 17 个文件都放在 `llvm-overlay/llvm/`，setup 会一次性应用。

另一个问题是原有文档建议：

```bash
build/bin/clang --target=cpu0 -S -emit-llvm ...
```

实际稳定路径是 LBD 使用的 MIPS triple，已同步改正到本仓库文档和示例。

## 5. 为什么不提交 `third_party` 和 `build`

LLVM 源码约 1.4 GB，build 目录约 1.3 GB。它们都在 `.gitignore` 中，只属于本机
开发环境。仓库提交的是：

- Cpu0 后端源码；
- 少量 LLVM overlay；
- Cpu0 回归测试；
- 最小示例；
- 中文学习笔记。

下次换机器时：

```bash
git clone https://github.com/ya0hu1/cpu0.git
./scripts/setup-llvm.sh
./scripts/build-cpu0.sh
./scripts/test-cpu0.sh
```

## 6. 下一步

Cpu0 基线已经可作为教学参照。下一步开始 `RiscvToy`，先做一个能被 `llc`
识别的最小 target，再逐步加入 RV32I 寄存器、调用约定和指令选择。
