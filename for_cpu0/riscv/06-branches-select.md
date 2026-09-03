# RiscvToy Stage 6：分支、比较和条件选择

## 1. 本阶段目标

Stage 5 后，RiscvToy 已经能处理函数调用和栈帧，但还不能编译控制流。本阶段补上：

```llvm
%c = icmp slt i32 %a, %b
br i1 %c, label %less, label %greater
```

以及：

```llvm
%r = select i1 %c, i32 %a, i32 %b
```

输出需要变成真正的 RISC-V 分支和跳转指令，例如：

```asm
blt a0, a1, .LBB0_2
...
j .LBB0_1
```

## 2. 新增的指令

### 2.1 比较类

RISC-V 的 `setcc` 指令不是设置“标志位”，而是把比较结果写成 0 或 1：

```text
slt  rd, rs1, rs2    # rd = signed(rs1 < rs2)
sltu rd, rs1, rs2    # rd = unsigned(rs1 < rs2)
slti rd, rs1, imm    # rd = signed(rs1 < imm)
sltiu rd, rs1, imm   # rd = unsigned(rs1 < imm)
xori rd, rs1, imm    # 配合 xori rd, rd, 1 做逻辑取反
```

为什么需要 `xori`？因为 RV32I 没有 `setge/setle` 指令，编译器会把：

```text
a >= b
```

改写成：

```text
!(a < b)
```

在 LLVM TableGen 中的一条对应 pattern 是：

```tablegen
def : Pat<(setge GPR:$rs1, GPR:$rs2),
          (RiscvToyXORI (RiscvToySLT GPR:$rs1, GPR:$rs2), 1)>;
```

### 2.2 B 型分支格式

条件分支指令使用 B 型编码：

```text
beq  rs1, rs2, target
bne  rs1, rs2, target
blt  rs1, rs2, target
bge  rs1, rs2, target
bltu rs1, rs2, target
bgeu rs1, rs2, target
```

B 型格式和 S 型一样需要把立即数拆分：

```tablegen
class RiscvToyB<...> {
  bits<12> imm12;
  let Inst{31}    = imm12{11};
  let Inst{30-25} = imm12{9-4};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-8}  = imm12{3-0};
  let Inst{7}     = imm12{10};
}
```

这是 RISC-V 指令格式里最容易混淆的字段之一，值得单独做一步来观察。

### 2.3 无条件跳转

`br label` 被匹配成教学阶段的伪指令：

```tablegen
def PseudoBR : RiscvToyPseudo<(outs), (ins brtarget:$BrDst),
                              "j\t$BrDst", [(br bb:$BrDst)]>;
```

真实编码阶段需要把它展开成 `jal x0, target`，也就是 J 型格式。

## 3. 直接分支 pattern

LLVM IR 的：

```llvm
br i1 %c, label %less, label %greater
```

在 SelectionDAG 中通常是 `brcond(setcc(lhs, rhs, cc))`。RiscvToy 直接给每种条件写
pattern：

```tablegen
def : Pat<(brcond (i32 (setlt GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBLT GPR:$rs1, GPR:$rs2, bb:$BrDst)>;
```

没有直接分支指令的条件通过交换操作数完成，例如：

```text
a > b
等价于
b < a
```

所以：

```tablegen
def : Pat<(brcond (i32 (setgt GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBLT GPR:$rs2, GPR:$rs1, bb:$BrDst)>;
```

## 4. `select` 为什么没有单条指令

RV32I 基础 ISA 没有条件移动指令。`select` 在机器码层必须变成一个“小三角控制流”：

```text
Head
  |
  +-- 条件为真 --> Tail
  |
  IfFalse
  |
  +--------------> Tail
```

实现分两步：

1. `LowerOperation()` 把 `select` 降低成自定义节点 `RiscvToyISD::SELECT_CC`；
2. TableGen 把这个节点匹配成 `RiscvToySelectPseudo`；
3. `EmitInstrWithCustomInserter()` 在伪指令处插入一个条件分支和 PHI。

伪指令定义：

```tablegen
let usesCustomInserter = 1 in
def RiscvToySelectPseudo
    : RiscvToyPseudo<(outs GPR:$dst),
                     (ins GPR:$lhs, GPR:$rhs, i32imm:$cc,
                      GPR:$truev, GPR:$falsev),
                     ...,
                     [(set GPR:$dst,
                       (RiscvToySelectCCFlag GPR:$lhs, GPR:$rhs,
                        (i32 imm:$cc), GPR:$truev, GPR:$falsev))]>;
```

伪指令的 6 个显式 operand 是：

```text
dst   选择结果寄存器
lhs   icmp 左操作数
rhs   icmp 右操作数
cc    条件码，例如 ISD::SETLT
truev 条件为真时选的值
falsev 条件为假时选的值
```

Custom inserter 创建两个新 MachineBasicBlock：

- `TailMBB`：放原来的后续指令，并把 `truev/falsev` 变成 PHI；
- `IfFalseMBB`：条件为假时走到这里，再落到 Tail。

最终 O2 下 `select a<b ? a : b` 输出类似：

```asm
blt a0, a1, .LBB0_2
addi a0, a1, 0
.LBB0_2:
ret
```

## 5. 小整数常量

为了支持 `phi [0, ...]` 和 `return 10` 这类常见源码，本阶段加入了最基础的常量
materialization：

```tablegen
def : Pat<(immSExt12:$imm), (RiscvToyADDI X0, immSExt12:$imm)>;
```

它把 `-2048..2047` 的常量编译成：

```asm
addi a0, zero, 10
```

更大的 32 位常量需要 `lui + addi` 或 `auipc + addi`，保留给 MC 阶段。

## 6. Branch analysis

SelectionDAG 只是把分支选出来，后续的 MachineIR 优化还需要能分析分支。因此
`RiscvToyInstrInfo` 补了四个基础接口：

```text
analyzeBranch()
insertBranch()
removeBranch()
reverseBranchCondition()
```

`Cond` 的表示是：

```text
[分支指令 opcode, rs1, rs2]
```

有了这些方法后，O2 的循环可以去掉多余的跳转：

```asm
addi a2, zero, 0
bge a2, a0, .LBB0_3
.LBB0_2:
add a1, a1, a2
addi a2, a2, 1
blt a2, a0, .LBB0_2
.LBB0_3:
addi a0, a1, 0
ret
```

## 7. 测试

新增测试：

```text
07-branch.ll
08-select.ll
09-loop.ll
10-setcc.ll
```

运行：

```bash
build/bin/llvm-lit -sv \
  third_party/llvm-project/llvm/test/CodeGen/RiscvToy
```

## 8. 当前边界

Stage 6 已支持：

- `eq/ne/slt/sle/sgt/sge` 与无符号版本；
- `br` 条件分支和无条件跳转；
- `select` 展开成分支和 PHI；
- `setcc` 结果作为 0/1 参与算术；
- 小整数常量；
- 基础 branch analysis，可以优化循环。

尚未支持：

- 超过 12 位的常量 materialization；
- jump table / switch；
- 间接跳转；
- B 型与 J 型的真实编码和 fixup；
- 分支超出 12 位编码范围后的重定位/长跳转。
