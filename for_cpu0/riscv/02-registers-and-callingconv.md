# RiscvToy Stage 2：寄存器模型与调用约定表

## 1. 本阶段目标

Stage 1 只是让 LLVM 认识 `riscvtoy`。Stage 2 把 RV32I 的整数寄存器表写进
TableGen，让 LLVM 能生成：

```text
RiscvToyGenRegisterInfo.inc
RiscvToyGenCallingConv.inc
```

这是后面写指令格式和调用低层代码之前的“地图”。

## 2. 新增文件

```text
backend/RiscvToy/
  RiscvToy.td
  RiscvToyRegisterInfo.td
  RiscvToyCallingConv.td
```

## 3. RiscvToy 寄存器定义

### 3.1 硬件寄存器

RISC-V 有 32 个通用整数寄存器：

```text
x0  x1  x2  x3  x4  x5  x6  x7
x8  x9  x10 x11 x12 x13 x14 x15
x16 x17 x18 x19 x20 x21 x22 x23
x24 x25 x26 x27 x28 x29 x30 x31
```

TableGen 中每个寄存器记录三样东西：

```tablegen
def X10 : RiscvToyReg<10, "x10", ["a0"]>, DwarfRegNum<[10]>;
```

- 硬件编码是 `10`，也就是机器码寄存器字段里的 `01010`；
- 汇编名字是 `x10`；
- ABI 别名是 `a0`；
- DWARF 寄存器编号是 `10`。

### 3.2 ABI 名字

寄存器分配器一般使用 `x0` 到 `x31` 这种稳定名字。汇编里为了可读性经常使用
ABI 别名：

```text
x0  zero
x1  ra
x2  sp
x3  gp
x4  tp
x5-x7, x28-x31   t0-t6
x8-x9            s0-s1
x10-x17          a0-a7
x18-x27          s2-s11
```

`AltNames` 只影响打印和解析。真正的编码仍然是寄存器编号。

### 3.3 分配顺序

TableGen 中 `GPR` 的注册顺序就是 LLVM 倾向于分配的顺序：

```text
先 caller-saved
  a0-a7, t0-t2, t3-t6
再 callee-saved
  s0-s1, s2-s11
```

这样做会让编译器优先使用调用者负责保存的寄存器，减少在函数出入口保存恢复
callee-saved 寄存器的次数。

### 3.4 为什么这些寄存器不能随便分配

`X0`（zero）不能进普通分配类：

- 读 `x0` 恒为 0；
- 写 `x0` 被硬件丢弃。

`X2`（sp）要单独处理：

- 栈指针由 frame lowering 管理；
- 寄存器分配器不应把普通值塞进 sp。

`X3` 和 `X4`（gp/tp）用于全局指针和线程指针，教学后端先保留。

> Stage 5 修正：为了让 `sw/lw` 能保存 `x1/ra` 并显式引用 `sp`，`GPR` 类后来会加入
> `x1,x2,x3,x4`；其中 `x2/x3/x4` 仍在 C++ 的 `getReservedRegs()` 中保留，
> `x1` 与真实 RISC-V 后端一样参与 callee-saved 管理。

所以代码里提供了三个类：

```text
GPR      普通可分配寄存器，不含 x0/x2/x3/x4
GPRX0    只含 x0，用于显式写 zero
GPRNoX0  不含 x0 的补充类
SP       只含 x2
```

## 4. 调用约定表

### 4.1 整数参数

标准 RV32I ILP32 约定：

```text
参数 1   -> a0 / x10
参数 2   -> a1 / x11
...
参数 8   -> a7 / x17
多余参数 -> 栈
返回值   -> a0 / x10
```

TableGen 中的 `CC_RiscvToy_ILP32` 就是这张表：

```tablegen
CCIfType<[i32], CCAssignToReg<[X10, X11, X12, X13,
                                X14, X15, X16, X17]>>,
```

`RetCC_RiscvToy_ILP32` 处理返回值。

### 4.2 Callee-saved 寄存器

按标准约定，被调用者负责保存：

```text
ra, gp, tp, s0-s11
```

即：

```text
x1, x3, x4, x8-x9, x18-x27
```

TableGen 中：

```tablegen
def CSR_ILP32 : CalleeSavedRegs<(add X1, X3, X4, X8, X9,
                                 (sequence "X%u", 18, 27))>;
```

LLVM 会由它生成两个 C++ 可用的数组：

```text
CSR_ILP32_SaveList
CSR_ILP32_RegMask
```

## 5. 如何验证

构建完成后，生成文件位于：

```text
build/lib/Target/RiscvToy/RiscvToyGenRegisterInfo.inc
build/lib/Target/RiscvToy/RiscvToyGenCallingConv.inc
```

可以检查寄存器枚举：

```bash
rg 'RiscvToy::X10|RiscvToy::GPRRegClass' \
  build/lib/Target/RiscvToy/RiscvToyGenRegisterInfo.inc
```

可以检查调用约定数组：

```bash
rg 'CSR_ILP32' \
  build/lib/Target/RiscvToy/RiscvToyGenCallingConv.inc
```

## 6. 下一步

有了寄存器，下一步是定义基础指令编码和 SelectionDAG pattern。最小目标是：

```llvm
define i32 @add(i32 %a, i32 %b) {
  %sum = add i32 %a, %b
  ret i32 %sum
}
```

能生成：

```asm
add a0, a0, a1
ret
```
