# RiscvToy Stage 11：移位指令

## 1. 本阶段目标

RiscvToy 已经支持算术、访存和常量，但 LLVM IR 的 `shl/lshr/ashr` 还不能选择。
本阶段加入 RV32I 的移位指令：

```text
立即数移位
  slli rd, rs1, shamt
  srli rd, rs1, shamt
  srai rd, rs1, shamt

变量移位
  sll rd, rs1, rs2
  srl rd, rs1, rs2
  sra rd, rs1, rs2
```

这样下面的 IR 可以编译：

```llvm
%x = shl i32 %a, 4
%y = lshr i32 %x, 5
%z = ashr i32 %y, 6
```

输出：

```asm
slli a0, a0, 4
srli a0, a0, 5
srai a0, a0, 6
```

## 2. RISC-V 移位立即数编码的特殊性

`slli/srli/srai` 在汇编语法上像 I 型指令，但字段并不完全等同于 `addi`：

```text
slli rd, rs1, shamt
```

真实编码：

```text
inst[31:25] = funct7
inst[24:20] = shamt
inst[19:15] = rs1
inst[14:12] = funct3
inst[11:7]  = rd
inst[6:0]   = opcode
```

其中 RV32I 的 `slli/srli` 使用：

```text
slli: funct7=0000000, funct3=001
srli: funct7=0000000, funct3=101
srai: funct7=0100000, funct3=101
```

`srai` 和 `srli` 的差异只在 funct7 的 bit30。这个字段同时决定算术右移还是逻辑右移。

## 3. TableGen 新格式

RiscvToy 原来的 `RiscvToyI` 把 `inst[31:20]` 整体当成 imm12。移位指令需要：

```text
funct7 + shamt
```

所以增加一个专门的格式：

```tablegen
class RiscvToyShiftI<bits<7> funct7, bits<3> funct3, bits<7> opcode, ...> {
  bits<5> shamt;
  bits<5> rs1;
  bits<5> rd;

  let Inst{31-25} = funct7;
  let Inst{24-20} = shamt;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7}  = rd;
  let Inst{6-0}   = opcode;
}
```

立即数指令定义：

```tablegen
def RiscvToySLLI : RiscvToyShiftI<0b0000000, 0b001, 0b0010011,
                                  ...,
                                  [(set GPR:$rd,
                                    (shl GPR:$rs1, uimm5:$shamt))]>;
```

`uimm5` 同时是 Operand 和 ImmLeaf，所以 AsmPrinter 能打印 `shamt`，
SelectionDAG 也知道它必须是一个 0..31 的常量：

```tablegen
def uimm5 : Operand<i32>,
            ImmLeaf<i32, [{ return isUInt<5>(Imm); }]> {
  let EncoderMethod = "getImmOpValue";
}
```

## 4. 变量移位

变量移位使用普通 R 型：

```tablegen
def RiscvToySLL : RiscvToyR<0b0000000, 0b001, 0b0110011,
                            ...,
                            [(set GPR:$rd, (shl GPR:$rs1, GPR:$rs2))]>;

def RiscvToySRL : RiscvToyR<0b0000000, 0b101, 0b0110011,
                            ...,
                            [(set GPR:$rd, (srl GPR:$rs1, GPR:$rs2))]>;

def RiscvToySRA : RiscvToyR<0b0100000, 0b101, 0b0110011,
                            ...,
                            [(set GPR:$rd, (sra GPR:$rs1, GPR:$rs2))]>;
```

LLVM IR 的移位数量作为第二个 i32 operand，和 RISC-V 的 rs2 匹配。

## 5. AsmParser/Disassembler 的同步

CodeGen 只是把 MachineInstr 发到 MC 层。为了让：

```text
llc -> .s
llvm-mc -> .o
llvm-objdump -d
```

继续闭环，需要让 parser/disassembler 也认识这些 mnemonic。

AsmParser 增加两组：

```text
sll/srl/sra   -> R 型，三个寄存器 operand
slli/srli/srai -> rd, rs1, shamt
```

Disassembler 在 opcode 0x33 中新增：

```cpp
case 1:
  Inst.setOpcode(RiscvToy::RiscvToySLL);
  break;
case 5:
  Inst.setOpcode((Insn & (1U << 30)) ? RiscvToy::RiscvToySRA
                                     : RiscvToy::RiscvToySRL);
  break;
```

在 opcode 0x13 中先判断是不是 shift-immediate：

```cpp
if (funct3 == 1 || funct3 == 5)
  Status = decodeShiftImmediate(Instr, Insn);
else
  Status = decodeIType(Instr, Insn);
```

移位数量从 `inst[24:20]` 取 5 位：

```cpp
unsigned Shamt = (Insn >> 20) & 0x1f;
```

## 6. 实测输出

```llvm
define i32 @var(i32 %a, i32 %n) {
  %x = shl i32 %a, %n
  %y = lshr i32 %x, %n
  %z = ashr i32 %y, %n
  ret i32 %z
}
```

汇编/object 反汇编：

```asm
sll a0, a0, a1
srl a0, a0, a1
sra a0, a0, a1
ret
```

## 7. 测试

新增：

```text
backend/test/CodeGen/RiscvToy/17-shifts.ll
backend/test/MC/RiscvToy/04-shifts.s
```

运行：

```bash
./scripts/test-cpu0.sh
```

当前结果：158 个 Cpu0/RiscvToy 测试全部通过。

## 8. 本阶段边界

Stage 11 已完成：

- `shl/lshr/ashr` 的立即数形式；
- `shl/lshr/ashr` 的寄存器变量形式；
- shift-immediate 的特殊 funct7/shamt 编码；
- parser、object、disassembler 都支持 6 条移位指令；
- `srai` 与 `srli` 的区别通过 funct7 bit30 正确处理。

尚未支持：

- 字节/半字 load/store：`lb/lh/lbu/lhu/sb/sh`；
- 全局/PC-relative 地址；
- 轮转/位操作扩展；
- 64 位移位。
