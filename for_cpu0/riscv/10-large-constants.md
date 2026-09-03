# RiscvToy Stage 10：32 位常量 materialization

## 1. 本阶段目标

Stage 6 只能 materialize `-2048..2047` 的常量，因为当时所有常量都通过：

```asm
addi a0, zero, imm12
```

遇到 `5000` 会直接：

```text
Cannot select: i32 = Constant<5000>
```

本阶段加入 U 型 `lui`，并让 SelectionDAG 把不能放进 12 位的常量拆成：

```asm
lui  a0, 1
addi a0, a0, 904
```

两条指令的结果是：

```text
4096 + 904 = 5000
```

## 2. RV32I 怎样合成任意 32 位常量

RV32I 没有一条能立即写入任意 32 位常量的指令，但组合 `lui + addi` 可以做到：

```text
lui  rd, imm20    # rd = imm20 << 12
addi rd, rd, imm12 # rd = rd + 符号扩展的 imm12
```

`lui` 的 20 位立即数放在高 20 位，所以可以覆盖：

```text
imm20 << 12
```

问题是低 12 位是 12 位有符号 `addi`，范围是 `-2048..2047`。不能简单取
`imm20 = Value >> 12`，否则 `5000` 会得到：

```text
imm20 = 5000 >> 12 = 1
low   = 5000 & 0xfff = 904
```

正好没问题。但考虑 `-5000`：

```text
-5000 & 0xfff = 0xec8 = 3784
```

3784 超过 `addi` 的 2047 上限。因此拆法必须考虑低 12 位会被符号扩展成负数：

```text
如果低 12 位 >= 0x800：
  addi 写入的值其实是 low - 4096
  所以先把完整值加上 0x800，再取高 20 位
```

这就是通用公式：

```text
hi20 = (Value + 0x800) >> 12
lo12 = 符号扩展(Value & 0xfff)
```

例如 `-5000`：

```text
(-5000 + 0x800) >> 12 = -1
lo12 = -904

lui  a0, 1048575    # 0xfffff，表示 -4096
addi a0, a0, -904    # -4096 - 904 = -5000
```

## 3. TableGen 中的 U 型指令

Stage 7 已经在 `RiscvToyInstrFormats.td` 里预留了 U 型：

```tablegen
class RiscvToyU<bits<7> opcode, ...> {
  bits<20> imm20;
  bits<5> rd;

  let Inst{31-12} = imm20;
  let Inst{11-7}  = rd;
  let Inst{6-0}   = opcode;
}
```

本阶段加入真指令：

```tablegen
def RiscvToyLUI : RiscvToyU<0b0110111,
                            (outs GPR:$rd), (ins uimm20:$imm20),
                            "lui", "$rd, $imm20", []>;
```

opcode `0b0110111` 是 RISC-V 的 `LUI`，`imm20` 是无符号的 20 位字段。

## 4. 常量拆分与 SDNodeXForm

SelectionDAG 的 TableGen pattern 不能直接写任意 32 位常量，需要先告诉它：

1. 什么常量走这个 pattern；
2. 高 20 位和低 12 位分别是什么。

RiscvToy 定义：

```tablegen
def immNonSExt12 : ImmLeaf<i32, [{ return !isInt<12>(Imm); }]>;
```

这个 ImmLeaf 只匹配超出 `addi` 范围的常量，避免破坏已经支持的小常量：

```asm
addi a0, zero, 7
```

接着定义两个 `SDNodeXForm`：

```tablegen
def LO12Sext : SDNodeXForm<imm, [{
  int64_t Imm = N->getSExtValue();
  int64_t Lo = Imm & 0xfff;
  if (Lo >= 0x800)
    Lo -= 0x1000;
  return CurDAG->getTargetConstant(Lo, SDLoc(N), N->getValueType(0));
}]>;

def HI20 : SDNodeXForm<imm, [{
  int64_t Imm = N->getSExtValue();
  int64_t Hi = (Imm + 0x800) >> 12;
  Hi &= 0xfffff;
  return CurDAG->getTargetConstant(Hi, SDLoc(N), N->getValueType(0));
}]>;
```

最后用嵌套 pattern 表示“先生成 lui，再生成 addi”：

```tablegen
def : Pat<(immNonSExt12:$imm),
          (RiscvToyADDI (RiscvToyLUI (HI20 imm:$imm)),
                        (LO12Sext imm:$imm))>;
```

它看起来像一条指令，实际上选择器会先选 `RiscvToyLUI` 的寄存器结果，再让
`RiscvToyADDI` 消费那个结果。

## 5. 为什么保留小常量路径

如果所有常量都走 `lui + addi`，也能工作，但代码会很浪费：

```asm
; 7 的理想形式
addi a0, zero, 7

; 全走 lui+addi 的不理想形式
lui  a0, 0
addi a0, a0, 7
```

所以 RiscvToy 保留：

```tablegen
def : Pat<(immSExt12:$imm), (RiscvToyADDI X0, immSExt12:$imm)>;
```

并把大常量 pattern 限制为 `!isInt<12>(Imm)`。

## 6. CodeGen 实测

输入：

```llvm
define i32 @positive() {
  ret i32 5000
}
```

输出：

```asm
lui a0, 1
addi a0, a0, 904
ret
```

负数：

```llvm
ret i32 -5000
```

输出：

```asm
lui a0, 1048575
addi a0, a0, -904
ret
```

边界 `-4096`：

```asm
lui a0, 1048575
addi a0, a0, 0
ret
```

这表示 `lui` 的 0xfffff 本身就是 `0xfffff000`，解释为有符号数就是 `-4096`。

## 7. 为什么 assembler/disassembler 也要同步

`llc -filetype=asm` 现在会输出 `lui`，所以：

- `llvm-mc` 必须能解析 `lui rd, imm20`；
- `llvm-objdump` 必须能解码 opcode `0x37`；
- 否则“IR -> .s -> .o -> disasm”的闭环会断。

AsmParser 增加：

```cpp
if (Mnemonic == "lui") {
  ...
  Inst.setOpcode(RiscvToy::RiscvToyLUI);
  Inst.addOperand(MCOperand::createReg(Rd));
  Inst.addOperand(getMCOperand(ImmExpr));
  return emit(Inst);
}
```

Disassembler 增加：

```cpp
case 0x37: // LUI
  Status = decodeLUI(Instr, Insn);
  break;
```

U 型立即数不是符号扩展，而是直接取 `inst[31:12]` 的 20 位无符号数：

```cpp
int64_t Imm = Insn >> 12;
```

## 8. 测试

新增：

```text
backend/test/CodeGen/RiscvToy/16-large-constants.ll
backend/test/MC/RiscvToy/03-lui.s
```

运行：

```bash
./scripts/test-cpu0.sh
```

当前结果：156 个 Cpu0/RiscvToy 测试全部通过。

## 9. 本阶段边界

Stage 10 已完成：

- 超出 12 位的常量不再导致 `Cannot select`；
- `lui + addi` 能表示任意 RV32 常量；
- 小常量仍然走单条 `addi x0`；
- object 编码、汇编器、反汇编器都能处理 `lui`；
- 正数、负数和 `-4096` 边界都有测试。

尚未支持：

- `auipc` PC-relative 地址；
- 全局变量地址 materialization；
- `li` 汇编 pseudo；
- `la`/`%hi/%lo` 等地址 modifier。
