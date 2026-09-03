# RiscvToy Stage 8：反汇编器

## 1. 本阶段目标

Stage 7 已经能生成 `.o` 文件，但当时的字节只能靠：

```bash
llvm-objdump -s -j .text file.o
```

手工对照十六进制。本阶段补上 `MCDisassembler`，让下面的命令可以直接把机器码恢复成
RiscvToy 汇编：

```bash
build/bin/llvm-objdump -d --triple=riscvtoy-unknown-unknown file.o
```

里程碑验证：

```text
00000000 /* add:*/
       0: 33 05 b5 00  add a0, a0, a1
       4: 67 80 00 00  ret
```

## 2. 新增文件

```text
backend/RiscvToy/
  CMakeLists.txt                       # add_subdirectory(Disassembler)
  Disassembler/
    CMakeLists.txt
    RiscvToyDisassembler.cpp
backend/test/CodeGen/RiscvToy/
  14-disassembly.ll
  15-disassembly-jump.ll
```

`CMakeLists.txt` 中用 `add_llvm_component_library` 生成
`LLVMRiscvToyDisassembler`，并挂到 `RiscvToy` component。这样 `llvm-objdump`
链接时会把 `LLVMInitializeRiscvToyDisassembler()` 放进初始化列表。

重新构建后：

```text
Registered Targets:
  cpu0     - CPU0 (32-bit big endian)
  cpu0el   - CPU0 (32-bit little endian)
  riscvtoy - Educational RV32 backend
```

`llvm-objdump --version` 开始列出 `riscvtoy`，说明反汇编器已经被通用 MC 工具看到。

## 3. Disassembler 在 LLVM 中的位置

反汇编与编码方向相反：

```text
汇编器 / 代码生成                反汇编器

MCInst                           MCInst
   |                                ^
   v                                |
MCCodeEmitter                   MCDisassembler
   |                                ^
   v                                |
32-bit instruction             32-bit instruction
```

`llvm-objdump` 对每个地址调用：

```cpp
DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                            ArrayRef<uint8_t> Bytes, uint64_t Address,
                            raw_ostream &CStream) const override;
```

实现者负责：

1. 从 `Bytes` 读一个小端 32 位字；
2. 判断 opcode/funct3/funct7，选择指令；
3. 把寄存器号恢复成 `RiscvToy::X0 + n` 这种 MCRegister；
4. 把编码字段恢复成有符号立即数；
5. 设置 `Size = 4`，把 MCInst 交回打印层。

RiscvToy 的注册代码：

```cpp
extern "C" LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeRiscvToyDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheRiscvToyTarget(),
                                         createRiscvToyDisassembler);
}
```

## 4. 为什么先手写 Decoder

上游 RISCV 使用 `-gen-disassembler` 生成大表，能处理几百条指令。RiscvToy 当前只有
RV32I 的十几个指令，手写 decode 更符合教学目标：

- 能看到每条指令的字段；
- 不会一下子引入复杂的 decoder table；
- 遇到指令格式错误时很容易定位。

`RiscvToyDisassembler.cpp` 按 opcode 分组：

```cpp
switch (Opcode) {
case 0x33: // OP
case 0x13: // OP-IMM
case 0x03: // LOAD
case 0x23: // STORE
case 0x67: // JALR
case 0x63: // BRANCH
case 0x6f: // JAL
}
```

每个 family 再检查 `funct3`，例如 R 型：

```text
funct3 = 000 -> add/sub（funct7 bit30 区分）
funct3 = 010 -> slt
funct3 = 011 -> sltu
funct3 = 100 -> xor
funct3 = 110 -> or
funct3 = 111 -> and
```

## 5. 立即数如何恢复

RISC-V 的立即数不是只读一个连续字段就行。

I 型最简单：

```text
imm[11:0] = inst[31:20]
```

需要把它从 12 位无符号字段符号扩展成负数。RiscvToy 中的 helper：

```cpp
static int64_t signExtend(uint64_t Value, unsigned Bits) {
  if (Value & (1ULL << (Bits - 1)))
    return static_cast<int64_t>(Value | (~0ULL << Bits));
  return static_cast<int64_t>(Value);
}
```

S 型 `sw` 的 12 位立即数被拆成两段：

```text
imm[11:5] = inst[31:25]
imm[4:0]  = inst[11:7]
```

Decoder 先拼回 12 位再符号扩展。

B 型分支的立即数最特殊，它缺少 bit0，字段散在指令中：

```text
inst[31]    -> offset[12]
inst[30:25] -> offset[10:5]
inst[11:8]  -> offset[4:1]
inst[7]     -> offset[11]
```

代码恢复：

```cpp
int64_t Imm = signExtend(
    ((Insn >> 31) & 0x1) << 12 |
    ((Insn >> 7) & 0x1) << 11 |
    ((Insn >> 25) & 0x3f) << 5 |
    ((Insn >> 8) & 0xf) << 1,
    13);
```

因为 offset bit0 恒为 0，这里编码时直接放 `offset[12:1]`，恢复时再左移补 0。

J 型 `jal` 类似，但它有 20 个编码位，逻辑偏移是 21 位：

```cpp
int64_t Imm = signExtend(
    ((Insn >> 31) & 0x1) << 20 |
    ((Insn >> 21) & 0x3ff) << 1 |
    ((Insn >> 20) & 0x1) << 11 |
    ((Insn >> 12) & 0xff) << 12,
    21);
```

## 6. 寄存器号怎么映射

RiscvToy 的 `RiscvToyGenRegisterInfo.inc` 中寄存器枚举是连续的：

```text
X0 = 1
X1 = 2
...
X31 = 32
```

因此 decoder 只需要：

```cpp
if (RegNo >= 32)
  return MCDisassembler::Fail;
Inst.addOperand(MCOperand::createReg(RiscvToy::X0 + RegNo));
```

打印层已经使用 ABI 名字，所以反汇编显示 `a0/ra/sp`，而不是 `x10/x1/x2`。

## 7. Pseudo 如何回到汇编

`PseudoRET` 不会出现在 object 中，它在编码时已经展开成：

```text
jalr x0, 0(ra)
```

Decoder 解码出真指令 `RiscvToyJALR`，AsmWriter 再通过：

```tablegen
def : InstAlias<"ret", (RiscvToyJALR X0, X1, 0), 4>;
```

打印成常见的 `ret`。因此 object 反汇编结果会保留 RISC-V 的易读别名。

## 8. 实测结果

### 8.1 简单算术

```text
00000000 /* add:*/
       0: 33 05 b5 00  add a0, a0, a1
       4: 67 80 00 00  ret
```

### 8.2 分支

```text
00000000 /* max:*/
       0: 63 54 b5 00  bge a0, a1, 8
       4: 13 85 05 00  addi a0, a1, 0
       8: 67 80 00 00  ret
```

这里的 `8` 是 PC 相对 offset，目标地址是 `0 + 8 = 8`。

### 8.3 函数调用

```text
       0: 13 01 01 ff  addi sp, sp, -16
       4: 23 26 11 00  sw ra, 12(sp)
       8: ef 00 00 00  jal ra, 0
       c: 83 20 c1 00  lw ra, 12(sp)
      10: 13 01 01 01  addi sp, sp, 16
      14: 67 80 00 00  ret
```

调用点显示 `jal ra, 0`，是因为 object 中外部 callee 只留下 `R_RISCV_JAL`
relocation，链接器还没填实际偏移。

## 9. 测试

新增：

```text
14-disassembly.ll
15-disassembly-jump.ll
```

运行：

```bash
build/bin/llvm-lit -sv \
  third_party/llvm-project/llvm/test/CodeGen/RiscvToy/14-disassembly.ll \
  third_party/llvm-project/llvm/test/CodeGen/RiscvToy/15-disassembly-jump.ll
```

`14` 验证 R/I/B 型和 `ret` 别名能反汇编；`15` 验证无条件跳转展开后的
`jal zero, 4` 能被解码回同一语义。

## 10. 本阶段边界

Stage 8 已完成：

- `llvm-objdump -d` 可以反汇编 RiscvToy object；
- RV32I 已实现指令按 opcode/funct 恢复；
- B/J 型立即数按 RISC-V 散位格式恢复；
- `jalr x0, 0(ra)` 打印成 `ret`；
- 当前全部 152 个 Cpu0/RiscvToy lit 测试通过。

尚未支持：

- 汇编器：`llvm-mc` 还不能把文本反向变成 MCInst/object；
- `lui/auipc` 大常量 materialization；
- 全局变量寻址；
- 指令解码器表生成；
- 反汇编结果中的符号/目标 label 美化。
