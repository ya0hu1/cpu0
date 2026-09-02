# Cpu0 后端构建笔记

## 1. 仓库结构与 LLVM 源码的关系

这个仓库不直接提交完整 LLVM 源码，因为 LLVM 太大。仓库保存的是：

```text
backend/Cpu0/         Cpu0 后端源码
llvm-overlay/llvm/    需要覆盖到 LLVM 的少量文件
scripts/              构建辅助脚本
```

`scripts/setup-llvm.sh` 会获取 LLVM 12 的固定提交：

```text
https://github.com/llvm/llvm-project
e8a397203c67adbeae04763ce25c6a5ae76af52c
```

然后：

1. 覆盖 `llvm-overlay/llvm/` 下的文件；
2. 把 `backend/Cpu0` 复制到 `llvm/lib/Target/Cpu0`；
3. 把测试复制到 `llvm/test/CodeGen/Cpu0`。

这样 LLVM 源码目录就变成一个带 Cpu0 后端的 LLVM。

## 2. 安装依赖

Ubuntu 22.04 下一般需要：

```bash
sudo apt update
sudo apt install build-essential git cmake ninja-build python3 zlib1g-dev \
  libzstd-dev libtinfo-dev
```

如果只想先准备文档和仓库，可以稍后再安装。

## 3. 获取 LLVM 源码并应用 Cpu0

```bash
./scripts/setup-llvm.sh
```

脚本会克隆到：

```text
third_party/llvm-project
```

该目录已在 `.gitignore` 中，不会进入本仓库。

## 4. 构建

```bash
./scripts/build-cpu0.sh
```

构建产物在：

```text
build/bin/llc
build/bin/clang
```

注意：第一次构建 LLVM + Clang 会比较久。

## 5. 一个最小验证

准备 `test/add.c`：

```c
int add(int a, int b) {
  return a + b;
}
```

生成 LLVM IR：

```bash
build/bin/clang --target=cpu0 -S -emit-llvm test/add.c -o add.ll
```

生成 Cpu0 汇编：

```bash
build/bin/llc -march=cpu0 add.ll -o add.s
```

如果 `llc` 能输出 Cpu0 汇编，说明后端已经接入。

## 6. 运行回归测试

```bash
./scripts/test-cpu0.sh
```

等价于：

```bash
ninja -C build check-llvm-codegen-cpu0
```

如果测试失败，优先检查：

1. LLVM 提交是否被改动；
2. `llvm-overlay` 是否成功覆盖；
3. `third_party/llvm-project/llvm/lib/Target/Cpu0` 是否存在；
4. TableGen 生成文件是否正常。
