# Homework 5 开发版 Checklist

来源：`descrip/hw5_des.md`

这份清单按实际 coding 顺序整理，目标是帮助你从 skeleton 稳定推进到可提交版本。

## 这份作业有什么作用
- [ ] 理解用户态程序和内核态模块如何协作
- [ ] 理解 Linux 系统调用在内核中的基本工作方式
- [ ] 练习 `fork()`、`exec*()`、`waitpid()` 这组经典进程控制接口
- [ ] 练习文件 I/O、标准输入读取、字符串和缓冲区处理
- [ ] 练习 Linux kernel module 的编译、加载、卸载和调试
- [ ] 理解“rootkit 隐藏行为”的核心思路是篡改观察结果，而不一定是删除真实对象
- [ ] 从防御视角理解攻击者可能如何隐藏文件、进程、模块和配置改动
- [ ] 建立一个关键心智模型：`ls`、`ps`、`cat`、`lsmod` 这些命令相信的是系统调用返回值，所以只要返回值被改写，用户态看到的世界就会被伪装

## 第 1 步：准备环境
- [ ] 只在课程要求的 Ubuntu 22.04 VM 中开发和测试
- [ ] 先建立 VM snapshot，避免内核模块 bug 把系统搞坏后无法恢复
- [ ] 安装必要工具，例如 `build-essential`
- [ ] 进入 `homework5-kit`，确认现有 skeleton 文件存在
- [ ] 确认最终只提交 3 个文件：`sneaky_process.c`、`sneaky_mod.c`、`Makefile`

## 第 2 步：先打通编译流程
- [ ] 检查 skeleton 的 `Makefile` 当前能编译 `sneaky_mod.ko`
- [ ] 修改 `Makefile`，让它也能编译用户态程序 `sneaky_process`
- [ ] 增加 `clean` 时删除用户态程序生成物
- [ ] 运行一次 `make`，先确保整体 build 流程没有结构问题
- [ ] 确认最终目标是同时得到 `sneaky_process` 和 `sneaky_mod.ko`

## 第 3 步：实现 `sneaky_process.c` 的最小版本
- [ ] 新建 `sneaky_process.c`
- [ ] 加入所需头文件，例如 `stdio.h`、`unistd.h`、`sys/types.h`、`sys/wait.h`、`fcntl.h`、`string.h`、`stdlib.h`
- [ ] 启动时打印 PID，格式必须严格符合题目要求：`sneaky_process pid = %d\n`
- [ ] 实现最小等待循环：从标准输入一次读一个字符
- [ ] 读到字符 `q` 时退出循环
- [ ] 单独测试这个最小版本，确认能打印 PID、能等待、能在输入 `q` 后退出

## 第 4 步：实现 `/etc/passwd` 的备份与恢复
- [ ] 写一个文件复制函数，把 `/etc/passwd` 备份到 `/tmp/passwd`
- [ ] 选择你要的实现方式，推荐直接用 `open/read/write/close` 自己拷贝
- [ ] 以追加方式打开 `/etc/passwd`
- [ ] 在末尾写入这一行，内容必须完全一致：`sneakyuser:abc123:2000:2000:sneakyuser:/root:bash`
- [ ] 在程序退出前，把 `/tmp/passwd` 复制回 `/etc/passwd`
- [ ] 单独验证这一步：运行后真实 `/etc/passwd` 被追加，退出后恢复原样

## 第 5 步：实现命令执行辅助函数
- [ ] 写一个辅助函数，负责 `fork()` 一个子进程
- [ ] 在子进程中使用 `exec*()` 执行命令
- [ ] 在父进程中使用 `waitpid()` 等待子进程结束
- [ ] 让这个辅助函数能返回执行是否成功，便于后续调试
- [ ] 先用这个函数跑通最简单的外部命令，确认 `fork/exec/waitpid` 逻辑正确

## 第 6 步：让 `sneaky_process` 能加载和卸载模块
- [ ] 用上一步的辅助函数执行 `insmod sneaky_mod.ko`
- [ ] 再实现执行 `rmmod sneaky_mod`
- [ ] 确认父进程会等待模块装载和卸载完成
- [ ] 暂时先不加隐藏功能，也可以先验证装载链路
- [ ] 再把当前进程 PID 作为模块参数传给 `insmod`
- [ ] 单独测试这一阶段，确认 `insmod` / `rmmod` 都成功

## 第 7 步：先读懂 skeleton 的 `sneaky_mod.c`
- [ ] 理解 `kallsyms_lookup_name("sys_call_table")` 的作用
- [ ] 理解 `enable_page_rw()` 和 `disable_page_rw()` 为什么需要存在
- [ ] 理解当前 skeleton 是怎么保存原始 `openat` 函数地址的
- [ ] 理解模块加载时如何替换 syscall table 项
- [ ] 理解模块卸载时如何恢复 syscall table 项
- [ ] 明确接下来还需要补的 hook 至少包括 `getdents64` 和 `read`

## 第 8 步：给模块加 PID 参数
- [ ] 在 `sneaky_mod.c` 里定义模块参数，用来接收 sneaky 进程的 PID
- [ ] 使用 `module_param(...)`
- [ ] 把 PID 保存成便于比较的形式，例如字符串形式
- [ ] 模块加载时打印收到的 PID 方便调试
- [ ] 验证 `insmod sneaky_mod.ko pid=...` 之类的调用方式能正常工作

## 第 9 步：先实现 `openat` 重定向
- [ ] 这是最适合先做通的隐藏功能，因为逻辑最直观
- [ ] 在 `sneaky_sys_openat` 中检查用户传入路径是否是 `/etc/passwd`
- [ ] 如果是，就把打开目标重定向到 `/tmp/passwd`
- [ ] 注意传给 syscall 的文件名指针在用户空间，必要时使用 `copy_to_user(...)`
- [ ] 其余情况调用原始 `openat`
- [ ] 单独测试这一功能：运行 sneaky 程序后，用 `cat /etc/passwd` 看是否显示的是原始文件内容

## 第 10 步：实现 `getdents64` hook 的基础版本
- [ ] 新增 `original_getdents64` 函数指针
- [ ] 新增 `sneaky_sys_getdents64` 包装函数
- [ ] 在模块加载时保存原始 `getdents64`，并替换为你的版本
- [ ] 在模块卸载时恢复原始 `getdents64`
- [ ] 调用原始 `getdents64` 拿到目录项缓冲区后，再开始过滤
- [ ] 阅读并理解 `man getdents64` 和 `struct linux_dirent64`
- [ ] 学会遍历返回的目录项缓冲区，每次按 `d_reclen` 前进

## 第 11 步：用 `getdents64` 隐藏 `sneaky_process`
- [ ] 在目录项缓冲区中过滤名字等于 `sneaky_process` 的项
- [ ] 学会两种常见删除方式：删除头项，或把后续内容前移覆盖当前项
- [ ] 更新最终返回的字节数
- [ ] 单独测试：`ls` 看不到 `sneaky_process`
- [ ] 单独测试：`find ... -name sneaky_process` 找不到结果

## 第 12 步：继续用 `getdents64` 隐藏 `/proc/<pid>`
- [ ] 在过滤逻辑中再加入一条：如果目录项名等于 sneaky 进程 PID 字符串，就把它隐藏
- [ ] 确认这条规则主要影响 `/proc` 目录读取结果
- [ ] 单独测试：`ls /proc` 看不到该 PID
- [ ] 单独测试：`ps -a -u <your_user_id>` 看不到 `sneaky_process`

## 第 13 步：实现 `read` hook 来隐藏模块自身
- [ ] 新增 `original_read` 函数指针
- [ ] 新增 `sneaky_sys_read` 包装函数
- [ ] 在模块加载时 hook `read`
- [ ] 在模块卸载时恢复 `read`
- [ ] 调用原始 `read` 后，检查当前读到的数据里是否包含 `sneaky_mod` 那一行
- [ ] 如果存在，就把那一行从返回缓冲区中删除
- [ ] 删除后更新返回字节数
- [ ] 单独测试：`lsmod` 看不到 `sneaky_mod`

## 第 14 步：把模块加载和测试流程串起来
- [ ] 启动 `sudo ./sneaky_process`
- [ ] 程序打印 PID
- [ ] 程序备份并修改 `/etc/passwd`
- [ ] 程序加载 `sneaky_mod.ko` 并传入 PID
- [ ] 程序进入等待循环
- [ ] 在另一个终端依次测试 `ls`、`find`、`ls /proc`、`ps`、`cat /etc/passwd`、`lsmod`
- [ ] 输入 `q` 结束等待
- [ ] 程序卸载模块并恢复 `/etc/passwd`

## 第 15 步：做系统化自测
- [ ] `make` 能成功
- [ ] `sudo ./sneaky_process` 能打印正确 PID
- [ ] `/tmp/passwd` 被创建
- [ ] 真实 `/etc/passwd` 被追加了 sneakyuser 行
- [ ] `cat /etc/passwd` 显示的是原始内容而不是修改后的内容
- [ ] `ls` 看不到 `sneaky_process`
- [ ] `find` 找不到 `sneaky_process`
- [ ] `ls /proc` 看不到 sneaky PID
- [ ] `ps -a -u <user>` 看不到 `sneaky_process`
- [ ] `lsmod` 看不到 `sneaky_mod`
- [ ] 输入 `q` 后模块被卸载
- [ ] `/etc/passwd` 被恢复到原始状态
- [ ] 模块卸载后系统行为恢复正常

## 第 16 步：提交前收尾
- [ ] 检查 `sneaky_process.c`、`sneaky_mod.c`、`Makefile` 文件名是否完全正确
- [ ] 不要提交 `.ko`、可执行文件、日志、截图、临时文件
- [ ] 从干净状态重新 `make` 一次
- [ ] 再完整跑一遍最关键的测试流程
- [ ] 最终打包为 `proj5_netid.zip`

## 调试时最容易卡住的点
- [ ] 模块参数名和 `insmod` 传参不一致
- [ ] hook 了 syscall 但模块卸载时没完全恢复
- [ ] `getdents64` 过滤时目录项指针移动错了，导致目录遍历异常
- [ ] 删除目录项后忘了更新返回长度
- [ ] 比较路径时没有正确处理用户空间指针
- [ ] `read` 过滤 `/proc/modules` 时字符串边界处理不严谨
- [ ] 修改 `sys_call_table` 前后页权限切换有问题
- [ ] 因为模块 bug 导致系统卡死，所以一定要依赖 snapshot
