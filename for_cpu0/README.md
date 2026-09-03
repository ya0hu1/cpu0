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
