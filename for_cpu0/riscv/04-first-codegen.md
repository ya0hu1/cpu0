# RiscvToy Stage 4：第一条 RISC-V 汇编输出

## 1. 本阶段目标

Stage 3 建立了“指令卡片”，但 LLVM 还不能真正编译。Stage 4 把这些卡片接入
LLVM 的 SelectionDAG 流程，并补上 MC 打印层，让 `llc` 第一次输出真实 RISC-V
汇编。

目标输入：

```llvm
define i32 @add(i32 %a, i32 %b) {
  %sum = add i32 %a, %b
  ret i32 %sum
}
```

目标输出：

```asm
add:
  add a0, a0, a1
  ret
```

现在这条命令已经可以运行：

```bash
build/bin/llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
  -filetype=asm -o - examples/cpu0/add.ll
```

## 2. 新增目录和文件

```text
backend/RiscvToy/
  RiscvToy.h
  RiscvToySubtarget.h
  RiscvToySubtarget.cpp
  RiscvToyRegisterInfo.h
  RiscvToyRegisterInfo.cpp
  RiscvToyInstrInfo.h
  RiscvToyInstrInfo.cpp
  RiscvToyFrameLowering.h
  RiscvToyFrameLowering.cpp
  RiscvToyISelLowering.h
  RiscvToyISelLowering.cpp
  RiscvToyISelDAGToDAG.cpp
  RiscvToyAsmPrinter.h
  RiscvToyAsmPrinter.cpp
  RiscvToySubtarget.td
  MCTargetDesc/
```

Stage 1 里“故意不支持 CodeGen”的重写被删掉了。TargetMachine 现在调用
LLVM 默认的 CodeGen pipeline。

## 3. 从 IR 到汇编的路径

LLVM 后端通常分这几步：

```text
LLVM IR
  -> SelectionDAG
  -> 指令选择（DAG -> MachineInstr）
  -> 寄存器分配
  -> PEI（栈帧）
  -> MachineInstr
  -> MCInst
  -> 汇编文本
```

对应到本阶段的文件：

```text
RiscvToyTargetLowering  形式参数和返回值的 ABI 处理
RiscvToyISelDAGToDAG    TableGen 指令选择
RiscvToyRegisterInfo    可分配/保留寄存器
RiscvToyInstrInfo       TargetInstrInfo 封装
RiscvToyAsmPrinter      MachineInstr -> MCInst
RiscvToyInstPrinter     MCInst -> 汇编文本
```

## 4. TargetMachine 接入

之前 TargetMachine 重写了：

```cpp
bool RiscvToyTargetMachine::addPassesToEmitFile(...) {
  return true; // 明确表示“不支持”
}
```

Stage 4 删除这个重写，改为实现：

```cpp
TargetPassConfig *RiscvToyTargetMachine::createPassConfig(PassManagerBase &PM);
```

PassConfig 指定使用自己的 DAG 指令选择器：

```cpp
bool RiscvToyPassConfig::addInstSelector() {
  addPass(createRiscvToyISelDag(getRiscvToyTargetMachine(), getOptLevel()));
  return false;
}
```

这样 LLVM 公共 CodeGen 管线会继续执行寄存器分配、栈帧和汇编打印。

## 5. Subtarget 和 RegisterInfo

`RiscvToySubtarget` 继承 TableGen 生成的
`RiscvToyGenSubtargetInfo`，把后端对象聚合起来：

```cpp
RiscvToyInstrInfo InstrInfo;
RiscvToyFrameLowering FrameLowering;
RiscvToyTargetLowering TLInfo;
```

RegisterInfo 由 InstrInfo 持有，避免重复创建。它实现了后端必须的几个查询：

```cpp
getReservedRegs()
getCalleeSavedRegs()
getCallPreservedMask()
getFrameRegister()
```

保留寄存器是：

```text
x0   zero，读恒为 0
x2   sp，栈指针
x3   gp，全局指针
x4   tp，线程指针
```

## 6. 调用约定实际落地

Stage 2 的调用约定表现在由 C++ 使用。

形式参数：

```text
参数 1 -> a0 / x10
参数 2 -> a1 / x11
...
参数 8 -> a7 / x17
```

LLVM 在函数入口把物理寄存器拷进虚拟寄存器，然后寄存器分配器尽量让参数留在
同一物理寄存器，所以 `add` 函数通常能直接得到：

```asm
add a0, a0, a1
```

返回值由 `LowerReturn` 放回 `a0`。空返回则只生成一条 `ret`。

## 7. ret 的两种表示

硬件没有单独的 `ret` 指令，RISC-V 标准写法是：

```asm
jalr x0, 0(x1)
```

其中 `x1` 就是 `ra`，保存函数返回地址。

后端内部使用 `PseudoRET` 描述“函数返回”。AsmPrinter 把它展开成真正的
`RiscvToyJALR`：

```asm
jalr zero, 0(ra)
```

InstPrinter 再通过 TableGen InstAlias 打印成大家更熟悉的：

```asm
ret
```

这样 CodeGen 里语义清楚，汇编文本又符合 RISC-V 惯例。

## 8. MC 层为什么必不可少

仅注册 TargetMachine 还不够。TargetMachine 初始化时需要：

```text
MCAsmInfo           .text / .globl / 注释符号等文本规则
MCInstrInfo         每条指令的机器信息
MCRegisterInfo       寄存器编号和 ABI 名
MCSubtargetInfo      CPU/特性位
MCInstPrinter        MCInst -> 汇编文本
AsmPrinter           MachineInstr -> MCInst
```

这些由 `MCTargetDesc/RiscvToyMCTargetDesc.cpp` 注册。

## 9. 测试

新增测试：

```text
backend/test/CodeGen/RiscvToy/
  01-target-registration.ll
  02-add.ll
  03-arithmetic.ll
```

运行：

```bash
./scripts/test-cpu0.sh
```

当前结果为 140 个 lit 测试全部通过，其中 RiscvToy 3 个。

## 10. 当前限制

这个里程碑能生成无栈、无调用的整数汇编，但它还没有：

- `call` 和函数间跳转；
- 栈上局部变量与溢出；
- `ra` 保存恢复；
- 分支、比较；
- 访存指令；
- ELF object 编码。

因此现在应该使用：

```bash
-filetype=asm
```

不要使用 `-filetype=obj`，它会因为没有 MCCodeEmitter/MCAsmBackend 而失败。

## 11. 下一步

下一阶段补齐 `lw/sw`、函数调用和栈帧，让后端能编译真正的调用链函数。
