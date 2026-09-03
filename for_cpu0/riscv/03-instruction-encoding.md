# RiscvToy Stage 3：RV32I 指令编码模型

## 1. 本阶段目标

Stage 2 已经描述了“有哪些寄存器”。Stage 3 回答另一个问题：

```text
一条 32 位 RISC-V 指令，在硬件层面长什么样？
```

本阶段只建立 TableGen 指令模型，LLVM 仍然不会真的生成 RiscvToy 汇编。
但有了这些指令定义，后面才能接入 SelectionDAG、寄存器分配和汇编打印。

## 2. 新增文件

```text
backend/RiscvToy/
  RiscvToyInstrFormats.td
  RiscvToyInstrInfo.td
```

同时更新：

```text
  RiscvToyRegisterInfo.td  增加 GPRFull
  RiscvToy.td              包含新指令文件
  CMakeLists.txt           生成指令表和 DAG 选择表
```

## 3. RV32I 的 R 型指令

加法 `add rd, rs1, rs2` 是 RISC-V 最典型的 R 型指令。32 位布局是：

```text
bit 31..25   funct7
bit 24..20   rs2
bit 19..15   rs1
bit 14..12   funct3
bit 11..7    rd
bit 6..0     opcode
```

`add` 的三个编码常量是：

```text
funct7 = 0000000
funct3 = 000
opcode = 0110011
```

在 TableGen 中写成：

```tablegen
class RiscvToyR<bits<7> funct7, bits<3> funct3, bits<7> opcode, ...> {
  bits<5> rs2;
  bits<5> rs1;
  bits<5> rd;

  let Inst{31-25} = funct7;
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
  let Inst{6-0} = opcode;
}
```

`bits<5>` 表示 5 位寄存器字段，正好能编码 `x0` 到 `x31`。

## 4. RV32I 的 I 型指令

`addi rd, rs1, imm12` 使用 I 型：

```text
bit 31..20   12 位立即数
bit 19..15   rs1
bit 14..12   funct3
bit 11..7    rd
bit 6..0     opcode
```

`simm12` 是一个 32 位整数 Operand，`immSExt12` 保证指令选择阶段只匹配
能放进 12 位有符号立即数的常量：

```tablegen
def simm12 : Operand<i32>;

def immSExt12 : ImmLeaf<i32, [{ return isInt<12>(Imm); }]>;
```

## 5. 本阶段加入了哪些指令

```text
R 型: add, sub, and, or, xor
I 型: addi
控制: jalr
Pseudo: ret
```

`add` 的 SelectionDAG pattern 是：

```tablegen
def RiscvToyADD : RiscvToyR<...,
                            (outs GPR:$rd),
                            (ins GPR:$rs1, GPR:$rs2),
                            "add", "$rd, $rs1, $rs2",
                            [(set GPR:$rd,
                              (add GPR:$rs1, GPR:$rs2))]>;
```

这告诉 LLVM：当 SelectionDAG 中出现

```text
i32 结果 = add(i32 左操作数, i32 右操作数)
```

时，可以生成一条 `RiscvToyADD`，汇编文本是

```text
add rd, rs1, rs2
```

## 6. GPRFull 的作用

之前普通 `GPR` 故意不包含 `x0`、`sp`、`gp`、`tp`。

但 `jalr x0, x1, 0` 是“返回”的真实机器指令，其中 `rd` 必须能写 `x0`：

```text
jr ra 的规范形式是：
  jalr x0, 0(ra)
```

所以新增了 `GPRFull`，表示 32 个架构寄存器。普通算术运算继续使用 `GPR`，
避免寄存器分配器把结果分配进 `x0`。

## 7. PseudoRET

`ret` 并不是 RISC-V 硬件里独立的一条指令，它是汇编语法糖：

```tablegen
def PseudoRET : RiscvToyPseudo<(outs), (ins), "ret", [(RiscvToyRetFlag)]>,
                PseudoInstExpansion<(RiscvToyJALR X0, X1, 0)>;
```

含义是：

```text
CodeGen 使用 PseudoRET
到 MC/汇编阶段时展开成
jalr x0, x1, 0
```

这样后端内部有一个清晰的“函数返回”指令，硬件编码又保持真实 RISC-V 格式。

## 8. 如何验证

运行：

```bash
./scripts/setup-llvm.sh
```

之后检查生成文件：

```bash
rg 'RiscvToy::RiscvToyADD|RiscvToy::PseudoRET' \
  build/lib/Target/RiscvToy/RiscvToyGenInstrInfo.inc
```

## 9. 下一步

指令表只是“卡片”。下一步要把它接入真正的 CodeGen 对象：

```text
Subtarget / RegisterInfo / InstrInfo / TargetLowering
```

然后才能打开 `RiscvToyTargetMachine` 的代码生成通道。
