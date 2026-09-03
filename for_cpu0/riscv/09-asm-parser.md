# RiscvToy Stage 9：汇编器 AsmParser

## 1. 本阶段目标

Stage 8 之后，RiscvToy 可以“向前编译成 object”也可以“向后反汇编”，但两条路之间还缺
一个环节：把人类写的 `.s` 汇编文本再变回机器码。

本阶段补上 `MCTargetAsmParser`，让下面的命令真正工作：

```bash
build/bin/llvm-mc -triple=riscvtoy-unknown-unknown \
  -filetype=obj -o simple.o simple.s
```

里程碑验证：

```bash
build/bin/llvm-objdump -d --triple=riscvtoy-unknown-unknown simple.o
```

输出：

```text
       0: 13 05 15 00  addi a0, a0, 1
       4: 67 80 00 00  ret
```

## 2. 新增/修改文件

```text
backend/RiscvToy/
  CMakeLists.txt                       # add_subdirectory(AsmParser)
  AsmParser/
    CMakeLists.txt
    RiscvToyAsmParser.cpp
backend/test/MC/RiscvToy/
  01-asm-basic.s
  02-asm-relocs.s
scripts/
  setup-llvm.sh                        # 同步 RiscvToy MC 测试
  test-cpu0.sh                         # 同时运行 CodeGen 和 MC 测试
```

`LLVMRiscvToyAsmParser` 注册后，`llvm-mc --version` 会列出 `riscvtoy`，通用汇编器
才能为这个 target 找到 parser。

## 3. MC 汇编器的主流程

```text
.s 文本
  |
  v
MCAsmLexer          把字符切成 token
  |
  v
MCTargetAsmParser   ParseInstruction(): 按指令语法收集 operand
  |
  v
MatchAndEmitInstruction(): 把 operand 组装成 MCInst
  |
  v
MCStreamer -> MCCodeEmitter -> .o
```

RiscvToy 的 parser 需要实现这些虚函数：

```cpp
bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc);
OperandMatchResultTy tryParseRegister(...);
bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                      SMLoc NameLoc, OperandVector &Operands);
bool ParseDirective(AsmToken DirectiveID);
bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                             OperandVector &Operands, MCStreamer &Out,
                             uint64_t &ErrorInfo, bool MatchingInlineAsm);
```

`ParseDirective()` 对 `.text/.globl/.type/.size/.cfi_*` 等通用指令返回 true，表示交给
平台/通用 parser。RiscvToy 自己只处理指令语法。

## 4. 为什么这里也先手写

上游 RISCV 的 AsmParser 大约有 2600 行，并且依赖 `-gen-asm-matcher` 生成匹配表。
那套系统能同时处理几十条指令、寄存器别名、mnemonic 冲突和 inline asm。

RiscvToy 当前只支持 20 条左右指令，所以本阶段先写一个“显式 switch”的 parser：

- 每一行汇编到哪里、有哪些 operand，都可以直接读到；
- 寄存器别名、memory 语法、pseudo 展开都集中在一处；
- 后续接入生成的 AsmMatcher 时，这个文件可以逐步替换成标准实现。

本阶段没有修改 `RiscvToy.td` 去生成 AsmMatcher，因为手写 parser 只需要：

```cpp
RegisterMCAsmParser<RiscvToyAsmParser> X(getTheRiscvToyTarget());
```

## 5. Operand 模型

`RiscvToyOperand` 继承 `MCParsedAsmOperand`，保存四种类型：

```text
Token      第一个 operand，通常是 mnemonic 本身
Register   a0/x10/ra/sp 等寄存器
Immediate  常量或 label 表达式
Memory     12(sp) 这种 base + offset
```

对 `MCParsedAsmOperand` 必须实现：

```cpp
isToken()
isReg()
isImm()
isMem()
getReg()
getStartLoc()
getEndLoc()
print()
```

手动 parser 不依赖 `addRegOperands()/addImmOperands()` 这类生成 matcher 用的方法，
而是由 `MatchAndEmitInstruction()` 自己读取 operand 内容。

## 6. 寄存器名怎么识别

RISC-V 汇编常用 ABI 名，也允许 `x0-x31`。RiscvToy 的 parser 直接做字符串映射：

```cpp
bool RiscvToyAsmParser::matchRegisterName(StringRef Name,
                                          unsigned &RegNo) {
  StringRef Lower = Name.lower();
  int Value = StringSwitch<int>(Lower)
                  .Case("x0", 0).Case("zero", 0)
                  .Case("x1", 1).Case("ra", 1)
                  ...
                  .Default(-1);
  ...
}
```

表中的 MCRegister 编码是：

```text
RegNo = RiscvToy::X0 + architecturalNumber
```

因为 `RiscvToy::X0` 在 LLVM 枚举里是 1，而架构编码是 0。传入 `RiscvToy::X0 + 10`
后，MCCodeEmitter 再通过寄存器信息得到 HWEncoding=10。

当前支持：

```text
zero/ra/sp/gp/tp
t0-t6, s0-s11, a0-a7
fp 作为 s0 的别名
```

## 7. Memory operand 的解析

RISC-V load/store 语法是：

```asm
sw  ra, 12(sp)
lw  a0, 0(sp)
```

这不是简单的一个寄存器加一个立即数，而是一个复合 operand。RiscvToy 用
`parseMemoryOperand()` 处理：

1. 先解析 offset（可以省略，缺省为 0）；
2. 要求出现 `(`；
3. 解析 base register；
4. 要求出现 `)`；
5. 生成一个 `Memory<base, offset>` operand。

因此在 `MatchAndEmitInstruction()` 中，`lw/sw/jalr` 的第二个 operand 是 memory：

```cpp
if (mem(2, BaseReg, Offset))
  return fail("memory operand expected");
```

然后按真实指令 operand 顺序构造 MCInst：

```text
lw rd, offset(rs1) -> opcode(rd, rs1, offset)
sw rs2, offset(rs1) -> opcode(rs2, rs1, offset)
```

## 8. 按 mnemonic 手工派发

`MatchAndEmitInstruction()` 拿到 `Operands[0]` 的 mnemonic 后，按族处理：

```text
ret                 -> PseudoRET
j/jal/call          -> 跳转/调用
add/sub/and/or/xor/... -> R 型
addi/slti/xori/...  -> I 型
lw/sw               -> memory 访问
beq/bne/...         -> B 型分支
jalr                -> I 型间接跳转
```

例如 R 型指令：

```cpp
if (reg(1, Rd) || reg(2, Rs1) || reg(3, Rs2))
  return fail("three register operands expected");

Inst.setOpcode(RiscvToy::RiscvToyADD);
Inst.addOperand(MCOperand::createReg(Rd));
Inst.addOperand(MCOperand::createReg(Rs1));
Inst.addOperand(MCOperand::createReg(Rs2));
```

立即数由 `getMCOperand()` 处理：

```cpp
if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr))
  return MCOperand::createImm(CE->getValue());
return MCOperand::createExpr(Expr);
```

常量立即数变成 `MCOperand::createImm()`，label/函数名变成表达式 operand，交给
fixup/relocation。

## 9. Pseudo 的汇编语法

汇编文本中常用的 pseudo 可以直接复用 CodeGen 里的定义：

```asm
j    label    -> PseudoBR
call callee   -> PseudoCALL
ret           -> PseudoRET
```

这三个 pseudo 在 object 输出时由 MCCodeEmitter 展开成：

```text
jal x0, target
jal x1, target
jalr x0, 0(ra)
```

因此手工写 `.s` 和 `llc -filetype=asm` 生成的文本使用同一套语法。

## 10. llc 与 llvm-mc 的往返

现在可以完整跑通：

```bash
# 1. LLVM IR -> .s
build/bin/llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
  -filetype=asm -o func.s func.ll

# 2. .s -> .o
build/bin/llvm-mc -triple=riscvtoy-unknown-unknown \
  -filetype=obj -o func.o func.s

# 3. .o -> 反汇编
build/bin/llvm-objdump -d --triple=riscvtoy-unknown-unknown func.o
```

对函数调用样例，反汇编会显示：

```text
addi sp, sp, -16
sw ra, 12(sp)
jal ra, 0        # call callee 留下的 R_RISCV_JAL
lw ra, 12(sp)
addi sp, sp, 16
ret
```

这说明：

- `call callee` 已被 AsmParser 解析；
- PseudoCALL 编码阶段展开成 `jal x1, target`；
- callee 是外部符号，所以 object 中留下 `R_RISCV_JAL callee` relocation。

## 11. 测试

新增：

```text
backend/test/MC/RiscvToy/
  01-asm-basic.s
  02-asm-relocs.s
```

`01` 验证基础 R 型/I 型/ret 从汇编生成 object；`02` 验证 memory、call 和
`R_RISCV_JAL` relocation。

测试脚本现在运行三个目录：

```bash
./scripts/test-cpu0.sh
```

当前结果：154 个 Cpu0/RiscvToy 测试全部通过。

## 12. 本阶段边界

Stage 9 已完成：

- `llvm-mc` 可以汇编 RiscvToy 文本；
- 寄存器 ABI 名和 `x0-x31` 都可以识别；
- `offset(base)` memory 语法可用；
- `ret/call/j` pseudo 在汇编文本与 object 之间保持一致性；
- `llc -filetype=asm` 的输出可以由 `llvm-mc` 重新汇编；
- MC 测试目录接入统一测试脚本。

尚未支持：

- TableGen 生成的 AsmMatcher 和自动诊断；
- 带 relocation modifier 的全局地址，如 `%hi/%lo`、`%pcrel_hi`；
- `li/la` 等更丰富的汇编 pseudo；
- `.option`、`.attribute` 等 RISC-V 汇编器指令；
- 大于 12 位立即数的自动拆分。
