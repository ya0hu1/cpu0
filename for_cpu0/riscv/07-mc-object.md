# RiscvToy Stage 7：目标文件、机器码编码与重定位

## 1. 本阶段目标

Stage 6 结束时，RiscvToy 已经可以把简单 C/LLVM IR 编译成 RISC-V 汇编文本：

```asm
add:
  add a0, a0, a1
  ret
```

但 `llc -filetype=obj` 还无法输出 ELF object。本阶段把后端补到 MC 层，让下面的命令
真正产出 32 位 RISC-V 指令字节：

```bash
build/bin/llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
  -filetype=obj -o /tmp/add.o add.ll
```

输出的 object 会使用 ELF 的 `EM_RISCV` machine number，因此也叫：

```text
elf32-littleriscv
```

## 2. 新增/修改的关键文件

```text
backend/RiscvToy/
  RiscvToyInstrFormats.td       # 增加 U/J 型格式模板
  RiscvToyInstrInfo.td          # 增加真指令 jal，调整分支 operand
  MCTargetDesc/
    RiscvToyFixupKinds.h        # 定义 branch/jal fixup 编号
    RiscvToyMCCodeEmitter.{h,cpp}  # 把 MCInst 编码成 32 位机器码
    RiscvToyAsmBackend.{h,cpp}  # 应用本地 fixup，生成目标文件字节
    RiscvToyELFObjectWriter.cpp # 输出 ELF 头与重定位
    RiscvToyMCTargetDesc.{h,cpp} # 注册 MCCodeEmitter / MCAsmBackend
```

同时新增了 lit 测试：

```text
backend/test/CodeGen/RiscvToy/
  11-object-file.ll
  12-object-relocs.ll
  13-object-pseudo-jump.ll
```

## 3. LLVM 中“汇编输出”和“目标文件输出”的分叉点

看代码生成时要注意，RiscvToy 有两条向下走的路径：

```text
llc -filetype=asm          llc -filetype=obj
          |                        |
   SelectionDAG              SelectionDAG
   MachineInstr             MachineInstr
          |                        |
   RiscvToyAsmPrinter       MCStreamer
   InstPrinter             MCCodeEmitter
          |                        |
   .s 文本                  MCAsmBackend
                              MCELFObjectWriter
                                        |
                                      .o 文件
```

Stage 4 先接入的是第一条路径，所以“能打印汇编”和“能编码机器码”是两件事。打印汇编时
可以直接输出伪指令 `j`、`call`、`ret`，因为 `.s` 是给人/汇编器看的；输出 object 时，
MC 层必须把它们展开成真实 RV32I 指令。

## 4. 为什么 TableGen 需要更精确的 operand 名

LLVM 的 `-gen-emitter` 会生成 `RiscvToyGenMCCodeEmitter.inc`。它根据每条指令的
`ins` 声明决定怎样取得操作数：

```tablegen
def RiscvToyBEQ : RiscvToyB<...,
                           (ins GPR:$rs1, GPR:$rs2, brtarget:$imm12),
                           ...>;
```

关键点：

- `simm12` 增加 `EncoderMethod = "getImmOpValue"`，让编码器知道普通立即数从 operand
  直接取；
- `brtarget` 增加 `EncoderMethod = "getBranchTargetOpValue"`，让分支目标走 fixup；
- 分支和 `jal` 的第 3 个 operand 必须写成 `brtarget:$immXX`，不能随意命名。

如果不给名字，生成器可能把符号 operand 误当成 `getMachineOpValue()` 来编码，最终出现
“Unsupported relocation” 或错误字段。

TableGen 中的对应函数：

```cpp
unsigned RiscvToyMCCodeEmitter::getBranchTargetOpValue(...) const {
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm() >> 1);

  assert(MO.isExpr() && "Expected an expression for a branch target");
  MCFixupKind Kind = MI.getOpcode() == RiscvToy::RiscvToyJAL
                         ? MCFixupKind(RiscvToy::fixup_riscvtoy_jal)
                         : MCFixupKind(RiscvToy::fixup_riscvtoy_branch);
  Fixups.push_back(MCFixup::create(0, MO.getExpr(), Kind, MI.getLoc()));
  return 0;
}
```

如果目标已经是整数（例如表驱动产生的相对偏移），直接右移一位返回；如果是符号表达式，
则产生一个 fixup，并让 AsmBackend/ObjectWriter 后续处理。

## 5. Pseudo 在编码阶段展开

RiscvToy 保留的伪指令和真实机器码的对应关系如下：

```text
伪指令         编码时展开
-------------------------------------------
br label       -> jal x0, label
call callee    -> jal x1, callee
ret            -> jalr x0, 0(ra)
```

为什么这样展开：

- RISC-V 无条件跳转 `jal rd, offset` 会写 `rd = pc + 4`。`jal x0, label`
  丢弃返回地址，等价于汇编器里的 `j label`；
- 函数调用需要返回地址，所以 `call callee` 展开成 `jal x1, callee`，
  `x1/ra` 保存的是返回地址；
- 返回时执行 `jalr x0, 0(ra)`，跳到 `ra` 指向的地址且不保存新返回地址。

核心代码在 `RiscvToyMCCodeEmitter::encodeInstruction()`：

```cpp
case RiscvToy::PseudoBR:
  expandPseudoBR(MI, OS, Fixups, STI);
  return;
case RiscvToy::PseudoCALL:
  expandPseudoCALL(MI, OS, Fixups, STI);
  return;
case RiscvToy::PseudoRET:
  expandPseudoRET(MI, OS, Fixups, STI);
  return;
```

展开时构造一个新的 `MCInst`，例如：

```cpp
MCInst JAL;
JAL.setOpcode(RiscvToy::RiscvToyJAL);
JAL.addOperand(MCOperand::createReg(RiscvToy::X0));
JAL.addOperand(MI.getOperand(0));  // 保留原分支目标
```

然后调用生成的 `getBinaryCodeForInstr()`。非 pseudo 的真实指令不需要手工编码，由
TableGen 自动生成字段拆分代码。

## 6. 什么是 Fixup

编码一条 `bge a0, a1, .LBB0_2` 时，分支目标地址可能还没有确定。MC 层不知道当前指令在
section 里的最终位置，也不知道目标 block 的地址，所以先把指令主体编码出来，同时记录：

```text
fixup offset   : 0（这条 4 字节指令内）
fixup expr     : 目标 label/符号表达式
fixup kind     : branch 或 jal
```

真正的目标距离由两类代码处理：

1. 本地符号：`MCAsmBackend::applyFixup()` 直接在生成的数据上填位；
2. 外部符号：不能立即填值，`MCELFObjectWriter` 把它转成 ELF relocation，例如
   `R_RISCV_JAL`，留给链接器处理。

RiscvToy 当前定义：

```cpp
enum Fixups {
  fixup_riscvtoy_branch = FirstTargetFixupKind,
  fixup_riscvtoy_jal,
  ...
};
```

## 7. B 型和 J 型立即数为什么要单独编码

B 型分支的编码字段不是简单顺序排列的 12 位立即数：

```text
inst[31]   = imm[12]
inst[30:25] = imm[10:5]
inst[11:8]  = imm[4:1]
inst[7]     = imm[11]
```

因此 `adjustFixupValue()` 要把 offset 拆到这些位置：

```cpp
case RiscvToy::fixup_riscvtoy_branch:
  ...
  unsigned Bit12 = (Offset >> 12) & 0x1;
  unsigned Bits10_5 = (Offset >> 5) & 0x3f;
  unsigned Bits4_1 = (Offset >> 1) & 0xf;
  unsigned Bit11 = (Offset >> 11) & 0x1;
  return (Bit12 << 31) | (Bits10_5 << 25) |
         (Bits4_1 << 8) | (Bit11 << 7);
```

注意 offset 仍按字节保存，因为 RV32 指令长度是 4，但 RISC-V 分支/JAL 偏移的低位恒为 0。

J 型 `jal` 的字段更特殊：

```text
inst[31]    = imm[20]
inst[30:21] = imm[10:1]
inst[20]    = imm[11]
inst[19:12] = imm[19:12]
```

这些“跳序”在 RISC-V 规格中是固定的。把它们集中放在
`RiscvToyAsmBackend::adjustFixupValue()` 里，和 TableGen 的编码格式保持一致。

## 8. ELF Object Writer

`RiscvToyELFObjectWriter` 继承 `MCELFObjectTargetWriter`，告诉通用 ELF writer：

```cpp
MCELFObjectTargetWriter(
    /*Is64Bit=*/false,
    OSABI,
    ELF::EM_RISCV,
    /*HasRelocationAddend=*/true)
```

因此文件头会显示 `Machine: EM_RISCV (0xF3)`，并且重定位使用 `.rela.*`（带 addend）。

fixup 到 relocation 的映射：

```text
fixup_riscvtoy_branch -> R_RISCV_BRANCH
fixup_riscvtoy_jal    -> R_RISCV_JAL
FK_Data_4             -> R_RISCV_32
```

最后两条是因为 RiscvToy 目前生成 `.eh_frame` 等数据，这些 32 位地址引用也需要 relocation。

## 9. 实测验证

### 9.1 生成一个 object

```bash
build/bin/llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
  -filetype=obj -o /tmp/branch.o backend/test/CodeGen/RiscvToy/07-branch.ll
build/bin/llvm-readobj --file-headers /tmp/branch.o
```

可以看到：

```text
Format: elf32-littleriscv
Arch: riscv32
Machine: EM_RISCV (0xF3)
```

### 9.2 查看 `.text` 字节

```bash
build/bin/llvm-objdump -s -j .text /tmp/branch.o
```

输出：

```text
Contents of section .text:
 0000 6354b500 13850500 67800000
```

对应关系：

```text
6354b500  -> bge a0, a1, +8
13850500  -> addi a0, a1, 0
67800000  -> jalr x0, 0(ra)     (ret)
```

这正好对应 Stage 6 的：

```asm
bge a0, a1, .LBB0_2
addi a0, a1, 0
ret
```

### 9.3 查看函数调用的 relocation

```bash
build/bin/llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
  -filetype=obj -o /tmp/call.o backend/test/CodeGen/RiscvToy/05-function-call.ll
build/bin/llvm-readobj -r --symbols /tmp/call.o
```

输出中的核心行：

```text
0x8 R_RISCV_JAL callee 0x0
```

这说明调用点 `call callee` 已展开成 `jal x1, callee`，但 callee 是外部函数，
编码器无法在目标文件内算出距离，所以交给 ELF relocation。

## 10. 测试

新增：

```text
11-object-file.ll
12-object-relocs.ll
13-object-pseudo-jump.ll
```

运行：

```bash
build/bin/llvm-lit -sv \
  third_party/llvm-project/llvm/test/CodeGen/RiscvToy/11-object-file.ll \
  third_party/llvm-project/llvm/test/CodeGen/RiscvToy/12-object-relocs.ll \
  third_party/llvm-project/llvm/test/CodeGen/RiscvToy/13-object-pseudo-jump.ll
```

`11` 检查 ELF machine、`.text` section 和 `.text` 字节；`12` 检查外部调用生成
`R_RISCV_JAL callee`；`13` 用 O0 保留无条件跳转，检查 `PseudoBR` 展开成
`jal x0, target` 后写入 `.text` 的字节 `6f004000`。

## 11. 本阶段边界

Stage 7 已完成：

- `llc -filetype=obj` 可以输出 ELF object；
- RISC-V 指令按 32 位小端字节写进 `.text`；
- `PseudoBR/PseudoCALL/PseudoRET` 编码时展开；
- 本地分支通过 AsmBackend fixup 填值；
- 外部函数调用通过 ELF relocation 表达；
- `.eh_frame` 数据引用可以输出 `R_RISCV_32`。

尚未支持：

- `llvm-mc` 从汇编文本解析 RiscvToy 指令；
- `llvm-objdump -d` 反汇编 RiscvToy；
- `lui/auipc` 大常量 materialization；
- 全局变量/`gp` 寻址；
- long call、分支范围外自动生成长跳转。

这些内容适合作为后续 Stage 8 继续补。
