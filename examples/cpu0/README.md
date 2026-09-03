# Cpu0 最小示例

这个目录放一个能直接验证 Cpu0 后端的最小例子。

## 直接使用 LLVM IR

仓库已经保存了 [add.ll](add.ll)，不需要 clang 就可以运行：

```bash
build/bin/llc -march=cpu0 examples/cpu0/add.ll -o -
```

预期看到类似：

```asm
add:
	addu	$2, $4, $5
	ret	$lr
```

解释：

- `$4`、`$5` 是 Cpu0/MIPS 风格的前两个整数参数寄存器；
- `$2` 是返回值寄存器；
- `addu $2, $4, $5` 执行 32 位加法；
- `ret $lr` 表示返回地址跳转。

## 从 C 生成 IR

编译 CPU0 的 C 源码时，教程通常让 clang 先用 MIPS triple 生成 LLVM IR，再用 `llc -march=cpu0` 做后端：

```bash
build/bin/clang -target mips-unknown-linux-gnu -S -emit-llvm \
  examples/cpu0/add.c -o /tmp/add.ll
build/bin/llc -march=cpu0 /tmp/add.ll -o /tmp/add.s
```

这是因为 Cpu0 是教学 CPU，Clang 没有维护完整的 `cpu0-unknown-*` 驱动和 sysroot；而 Cpu0 的 32 位整数调用约定刻意仿照 MIPS，所以借用 MIPS triple 做前端最接近 LBD 原教程的做法。
