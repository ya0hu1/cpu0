# RiscvToy Stage 12：字节与半字访存

## 1. 本阶段目标

RiscvToy 之前只有 `lw/sw` 两个 32 位访存指令，下面的 IR 会无法选择：

```llvm
%v = load i8, i8* %p
```

本阶段加入 RV32I 的窄访存：

```text
有符号扩展 load
  lb  rd, offset(rs1)   # 8 位，符号扩展到 32 位
  lh  rd, offset(rs1)   # 16 位，符号扩展到 32 位

无符号扩展 load
  lbu rd, offset(rs1)   # 8 位，零扩展到 32 位
  lhu rd, offset(rs1)   # 16 位，零扩展到 32 位

窄 store
  sb  rs2, offset(rs1)  # 存低 8 位
  sh  rs2, offset(rs1)  # 存低 16 位
```

## 2. 为什么 load 要区分 signed/unsigned

RISC-V 没有“只读一个字节但扩展方式未知”的指令：

- `lb` 会把最高位当作符号位，读到负数；
- `lbu` 会补 0，读到 0..255。

LLVM IR 有三种窄 load 用法：

```text
sextloadi8   读 i8 后必须符号扩展到 i32
zextloadi8   读 i8 后必须零扩展到 i32
extloadi8    anyext，高 24 位不关心
```

因此：

```text
sextloadi8  -> lb
zextloadi8  -> lbu
extloadi8   -> lb（高 24 位不关心）

sextloadi16 -> lh
zextloadi16 -> lhu
extloadi16  -> lh
```

`extloadi8` 为什么可以用 `lb`？因为 anyext 的结果只有在高 24 位不会被使用时才合法；
此时符号扩展和零扩展都能提供正确的低 8 位。

## 3. TableGen 指令定义

窄 load 和 `lw` 一样使用 I 型格式，只是 funct3 不同：

```tablegen
def RiscvToyLB  : RiscvToyI<0b000, 0b0000011, ...>;
def RiscvToyLH  : RiscvToyI<0b001, 0b0000011, ...>;
def RiscvToyLW  : RiscvToyI<0b010, 0b0000011, ...>;
def RiscvToyLBU : RiscvToyI<0b100, 0b0000011, ...>;
def RiscvToyLHU : RiscvToyI<0b101, 0b0000011, ...>;
```

窄 store 和 `sw` 一样使用 S 型格式：

```tablegen
def RiscvToySB : RiscvToyS<0b000, 0b0100011, ...>;
def RiscvToySH : RiscvToyS<0b001, 0b0100011, ...>;
def RiscvToySW : RiscvToyS<0b010, 0b0100011, ...>;
```

## 4. SelectionDAG Pattern

本阶段增加两个 pattern 模板，覆盖寄存器/FrameIndex 和 `base + imm` 寻址：

```tablegen
multiclass RiscvToyLdPat<PatFrag LoadOp, Instruction Inst> {
  def : Pat<(LoadOp GPR:$rs1), (Inst GPR:$rs1, 0)>;
  def : Pat<(LoadOp AddrFI:$rs1), (Inst AddrFI:$rs1, 0)>;
  ...
}

multiclass RiscvToyStPat<PatFrag StoreOp, Instruction Inst> {
  ...
}
```

再批量实例化：

```tablegen
defm : RiscvToyLdPat<sextloadi8, RiscvToyLB>;
defm : RiscvToyLdPat<extloadi8, RiscvToyLB>;
defm : RiscvToyLdPat<zextloadi8, RiscvToyLBU>;
defm : RiscvToyLdPat<sextloadi16, RiscvToyLH>;
defm : RiscvToyLdPat<extloadi16, RiscvToyLH>;
defm : RiscvToyLdPat<zextloadi16, RiscvToyLHU>;

defm : RiscvToyStPat<truncstorei8, RiscvToySB>;
defm : RiscvToyStPat<truncstorei16, RiscvToySH>;
```

store 的 `truncstorei8/truncstorei16` 表示“寄存器里有完整 i32，只把低 8/16 位写回
内存”，正好对应 RISC-V 的 `sb/sh`。

## 5. AsmParser/Disassembler 同步

Memory 类 mnemonic 列表从两个扩展到：

```text
lb, lbu, lh, lhu, lw, sb, sh, sw, jalr
```

`jalr` 的 `offset(rs1)` 在语法上也是 memory，所以也留在这里。

Disassembler 不再只接受 funct3=010：

```cpp
LOAD 根据 funct3：
  000 -> lb
  001 -> lh
  010 -> lw
  100 -> lbu
  101 -> lhu

STORE 根据 funct3：
  000 -> sb
  001 -> sh
  010 -> sw
```

## 6. 实测输出

输入：

```llvm
%r = zext i8 %v to i32
```

输出：

```asm
lbu a0, 0(a0)
ret
```

输入：

```llvm
%r = sext i16 %v to i32
```

输出：

```asm
lh a0, 0(a0)
ret
```

写回：

```asm
sb a1, 0(a0)
sh a1, 0(a0)
```

## 7. 测试

新增：

```text
backend/test/CodeGen/RiscvToy/18-byte-half.ll
backend/test/MC/RiscvToy/05-load-store-sizes.s
```

运行：

```bash
./scripts/test-cpu0.sh
```

当前结果：160 个 Cpu0/RiscvToy 测试全部通过。

## 8. 本阶段边界

Stage 12 已完成：

- `lb/lbu/lh/lhu` 窄 load；
- `sb/sh` 窄 store；
- signed/unsigned 扩展选择正确；
- stack FrameIndex 和寄存器寻址都可用；
- object、assembler、disassembler 全部同步。

尚未支持：

- 全局符号地址，例如 `la t0, symbol`；
- 非对齐访存策略；
- 原子指令；
- volatile 之外更完整的 memory model 指令。
