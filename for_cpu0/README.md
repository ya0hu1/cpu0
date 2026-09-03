# for_cpu0 学习笔记

这里记录逐步理解 LLVM cpu0 后端的中文笔记。

## 目录

- [01-overview.md](01-overview.md)：LLVM 后端与 LBD cpu0 工程总览
- [02-build.md](02-build.md)：本仓库的获取、构建和测试方法
- [03-riscv-roadmap.md](03-riscv-roadmap.md)：从 cpu0 过渡到 RISC-V 后端的分阶段路线
- [04-llvm-overlay.md](04-llvm-overlay.md)：`llvm-overlay` 中每个文件的用途
- [05-cpu0-baseline-result.md](05-cpu0-baseline-result.md)：Cpu0 基线构建与验证结果
- [riscv/01-minimal-target.md](riscv/01-minimal-target.md)：RiscvToy Stage 1 最小 target 注册
- [riscv/02-registers-and-callingconv.md](riscv/02-registers-and-callingconv.md)：RiscvToy 寄存器模型与调用约定表
- [riscv/03-instruction-encoding.md](riscv/03-instruction-encoding.md)：RiscvToy Stage 3 RV32I 指令编码表
- [riscv/04-first-codegen.md](riscv/04-first-codegen.md)：RiscvToy Stage 4 首个 RISC-V 汇编输出
- [riscv/05-function-call-frame.md](riscv/05-function-call-frame.md)：RiscvToy Stage 5 函数调用与栈帧
- [riscv/06-branches-select.md](riscv/06-branches-select.md)：RiscvToy Stage 6 分支、比较和条件选择
- [riscv/07-mc-object.md](riscv/07-mc-object.md)：RiscvToy Stage 7 目标文件、机器码编码与重定位
- [riscv/08-disassembler.md](riscv/08-disassembler.md)：RiscvToy Stage 8 反汇编器
- [riscv/09-asm-parser.md](riscv/09-asm-parser.md)：RiscvToy Stage 9 汇编器 AsmParser
- [riscv/10-large-constants.md](riscv/10-large-constants.md)：RiscvToy Stage 10 32 位常量 materialization
- [riscv/11-shifts.md](riscv/11-shifts.md)：RiscvToy Stage 11 移位指令
- [riscv/12-byte-half-loads-stores.md](riscv/12-byte-half-loads-stores.md)：RiscvToy Stage 12 字节与半字访存
- [riscv/13-indirect-calls.md](riscv/13-indirect-calls.md)：RiscvToy Stage 13 间接函数调用
