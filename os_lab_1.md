# 操作系统专题实践 - 实验1

**71123329 段嘉文**

## Linux进程管理及其扩展

[TOC]

### 实验目的

通过本次实验，深入理解操作系统中进程管理的核心机制，掌握进程控制块（task_struct）、进程队列等关键数据结构在 Linux 内核中的实现方式。进一步通过扩展内核功能，实现自定义系统调用，提升对系统调用机制、进程调度与进程可见性控制的理解，增强内核级编程能力。

### 实验内容

1. 阅读并分析 Linux 5.13.10 内核源代码，重点研究 `task_struct` 结构体以及进程链表的组织形式，理解进程在内核中的表示与管理方式。
2. 在 Linux 内核中添加一个新的系统调用 `hide(pid_t pid)`，该系统调用可根据传入的进程 ID 将指定进程从进程列表中移除，使其不再被用户态工具（如 `ps`、`top` 等）所列举，从而实现进程隐藏功能。
3. 编写用户态测试程序，验证系统调用的功能正确性。

### 具体要求

- 修改内核源码，在合适位置实现 `sys_hide` 系统调用函数，并注册至系统调用表。
- 隐藏操作应通过将目标进程从全局进程链表（`init_task.tasks`）中解链实现，确保其不会被遍历到。
- 提供反隐藏机制（可选扩展），或通过重启恢复进程可见性。
- 编译并启动修改后的内核，确保系统基本功能正常运行。
- 测试新系统调用的行为，验证 `ps` 和 `top` 命令无法显示被隐藏的进程。
- 记录实验过程，撰写完整报告，包含代码说明、设计思路、测试结果及分析。

### 设计思路

本实验的设计基于 Linux 内核中进程管理的数据结构与模块化控制思想。传统的进程隐藏方法通常依赖于从 `tasks` 链表中移除目标进程节点，从而使其无法被 `ps`、`top` 等工具枚举。然而，该方法存在恢复困难、易破坏链表完整性等问题。为此，本实验采用一种更为安全、可控的设计方案：不直接修改进程链表，而是通过状态标记结合遍历过滤的方式实现逻辑上的“隐藏”。

1. **引入隐藏标志字段（hidden_flag）：**
   在 `task_struct` 结构体中添加一个自定义字段 `int hidden_flag;`，用于标识该进程是否应被隐藏。初始值为 0，设置为 1 表示该进程处于隐藏状态。
2. **实现系统调用 `sys_hide(pid_t pid)`：**
   用户可通过该系统调用传入指定 PID，内核查找对应进程并将其 `hidden_flag` 标记为 1。查找过程使用 `find_vpid()` 和 `pid_task()` 组合完成，确保正确获取 `task_struct` 指针。
3. **通过 `/proc` 文件系统提供运行时控制接口：**
   - 创建 `/proc/hide` 节点，支持读写操作：
     - 读取：返回当前全局隐藏功能开关状态（1 表示启用过滤，0 表示关闭）。
     - 写入：允许用户态通过 `echo "0" | sudo tee /proc/hide` 等方式动态启停隐藏功能。
   - 创建 `/proc/hidden_process` 节点，用于列出当前所有 `hidden_flag == 1` 的进程 PID，便于调试与监控。
4. **在进程列举路径中插入过滤逻辑：**
   修改内核中与 `/proc` 文件系统相关的进程遍历函数（如 `proc_pid_readdir` 或 `proc_fill_cache`），在返回每个进程信息前检查其 `hidden_flag` 状态及全局开关。若开关开启且目标进程被标记，则跳过该进程，使其不出现在 `/proc` 目录下，进而不会被 `ps`、`top` 等工具读取。
5. **用户态测试程序验证功能：**
   编写两个测试程序：
   - `hide_test.c`：接收 PID 参数，调用 `sys_hide()` 将其标记为隐藏。
   - `hide_user_processes_test.c`：批量隐藏特定条件下的用户进程。 测试流程包括：
   - 执行前后的 `ps aux` 对比；
   - 使用 `dmesg` 输出调试日志确认系统调用执行情况；
   - 动态启停隐藏功能并观察进程可见性变化。
6. **权限与安全性控制：**
   所有涉及系统调用和 `/proc` 写操作均需 `CAP_SYS_ADMIN` 权限，确保只有 root 用户可以修改隐藏状态，防止普通用户滥用。
7. **优势与特点：**
   - **非侵入式设计**：不修改 `tasks` 链表，避免破坏内核数据结构导致崩溃。
   - **可逆性强**：通过清除 `hidden_flag` 即可恢复可见，无需重启。
   - **动态可控**：通过 `/proc/hide` 实现全局开关，灵活应对调试与部署需求。
   - **可观测性好**：通过 `/proc/hidden_process` 可审计当前隐藏进程列表。

### 主要数据结构及其说明

本实验主要涉及以下核心数据结构和变量的修改与扩展：

1.`task_struct` 结构体扩展

`task_struct` 是 Linux 内核中用于描述进程的核心数据结构，定义在 `include/linux/sched.h` 中。本实验对该结构体进行了扩展，新增了 `cloak` 字段：

```c
struct task_struct {
    // ... existing fields ...
    int cloak;  // 进程隐藏标志：0表示正常显示，1表示隐藏
    // ... existing fields ...
};
```

**字段说明：**
- **cloak**：整型标志位，用于标识当前进程是否应被隐藏
  - 值为 `0`：进程正常显示，可被 `/proc` 遍历和用户态工具（如 `ps`、`top`）检测到
  - 值为 `1`：进程处于隐藏状态，在全局开关 `hidden_flag` 启用时不会出现在进程列表中
- **初始化时机**：在 `kernel/fork.c` 的 `copy_process()` 函数中，每个新创建的子进程的 `cloak` 字段被初始化为 `0`，确保新进程默认可见

**2. 全局隐藏开关 `hidden_flag`**

定义在 `fs/proc/hide.c` 中的全局变量，用于控制进程隐藏功能的整体启停状态：

```c
int hidden_flag = 1;  // 全局隐藏功能开关
EXPORT_SYMBOL(hidden_flag);  // 导出符号供其他模块使用
```

**变量说明：**
- **作用域**：通过 `EXPORT_SYMBOL` 导出，可被内核其他模块（如 `fs/proc/base.c`）访问
- **初始值**：设为 `1`，表示隐藏功能默认启用
- **运行时控制**：通过 `/proc/hide` 接口可动态修改，实现隐藏功能的开关切换
- **判断逻辑**：只有当 `hidden_flag == 1` 且进程的 `cloak == 1` 时，该进程才会被过滤

**3. 进程过滤机制相关函数**

在 `fs/proc/base.c` 中修改了两个关键函数，实现进程列表的过滤逻辑：

**（1）`proc_pid_readdir` 函数**
- **作用**：遍历 `/proc` 目录时，控制哪些进程 PID 目录应被列举
- **过滤逻辑**：在返回进程信息前检查 `hidden_flag` 和目标进程的 `cloak` 字段，若两者均为 `1` 则跳过该进程，不将其加入目录列表

**（2）`proc_pid_lookup` 函数**
- **作用**：通过 PID 直接访问 `/proc/[pid]` 目录时的查找函数
- **过滤逻辑**：即使用户知道确切的 PID 并尝试直接访问，若进程被标记为隐藏且全局开关开启，也会返回"无此文件或目录"错误

**4. 系统调用相关结构**

**（1）系统调用表项**

在 `arch/x86/entry/syscalls/syscall_64.tbl` 中注册了两个新的系统调用号：

| 调用号 | 架构 | 调用名称 | 内核函数 |
|--------|------|----------|----------|
| 447 | common | hide | sys_hide |
| 448 | common | hide_user_processes | sys_hide_user_processes |

**（2）系统调用函数签名**

```c
// 单进程隐藏控制
asmlinkage long sys_hide(pid_t pid, int on);

// 批量进程隐藏控制
asmlinkage long sys_hide_user_processes(uid_t uid, char *binname, int recover);
```

**5. `/proc` 接口数据结构**

**（1）`hide_proc_ops` 结构体**

定义在 `fs/proc/hide.c` 中，用于 `/proc/hide` 文件的操作接口：

```c
static const struct proc_ops hide_proc_ops = {
	.proc_write = proc_write_hidden,
	.proc_read  = proc_read_hidden,
	.proc_lseek = default_llseek,
};
```

**（2）`hidden_process_proc_ops` 结构体**

定义在 `fs/proc/hidden_process.c` 中，用于 `/proc/hidden_process` 文件的操作接口：

```c
static const struct proc_ops hidden_process_proc_ops = {
    .proc_read = proc_read_hidden_process,
};
```

**6. 数据结构之间的关系**

本实验中各数据结构的协作关系如下：

1. **进程创建时**：`copy_process()` 初始化新进程的 `task_struct.cloak = 0`
2. **隐藏操作时**：用户调用 `sys_hide()` 或 `sys_hide_user_processes()`，内核修改目标进程的 `task_struct.cloak = 1`，并调用 `proc_flush_pid()` 刷新缓存
3. **进程遍历时**：`proc_pid_readdir()` 和 `proc_pid_lookup()` 读取 `hidden_flag` 和 `task_struct.cloak`，决定是否过滤该进程
4. **运行时控制**：通过 `/proc/hide` 修改 `hidden_flag`，实现全局开关；通过 `/proc/hidden_process` 查询所有 `cloak == 1` 的进程列表

这种设计通过在进程描述符中添加轻量级标志位，结合全局开关和 `/proc` 过滤机制，实现了高效、安全、可逆的进程隐藏功能，避免了对内核核心数据结构的破坏性修改。

### 编译 Linux 内核

1. 在VirtualBox里安装Ubuntu，保证足够的磁盘空间（我当前分配的空闲空间约27GB）。

2. 下载Linux源码包并解压：

   ```shell
   mkdir -p os_lab
   cd os_lab
   wget https://mirrors.tuna.tsinghua.edu.cn/kernel/linux/kernel/v5.x/linux-5.13.10.tar.xz
   xz -d linux-5.13.10.tar.xz
   tar -xavf linux-5.13.10.tar
   ```

   得到`linux-5.13.10`目录。

3. 编辑配置文件，`make localmodconfig`会自动读取当前正在运行的 Ubuntu 里实际加载的所有模块，避免了全部编译Linux所带来的内存占用大以及花费时间长的弊端。（最开始没有做这一步，而是编译了整个包，发现十个小时都没有成功，而且磁盘空间也被全部占满，故这一步及其重要，一定要砍去不需要的包！）此处配置方式为在命令行中输入命令进行配置。

   ```shell
   cd linux-5.13.10
   # 清理之前残留的编译文件
   make mrproper
   
   # 包含现在正在运行的模块
   make localmodconfig
   
   # 禁用内核模块签名验证
   scripts/config --disable SYSTEM_TRUSTED_KEYS
   scripts/config --disable SYSTEM_REVOCATION_KEYS
   scripts/config --disable MODULE_SIG
   
   # 可选：禁用 BTF 调试信息（我的系统没有安装 pahole）
   scripts/config --disable DEBUG_INFO_BTF
   
   # 检查关键配置是否启用
   scripts/config --state PROC_FS        # 应该是 y
   scripts/config --state SYSCTL         # 应该是 y
   scripts/config --state MODULES        # 应该是 y
   
   # 更新配置文件
   make olddefconfig
   
   # 修改版本号
   nano Makefile
   EXTRAVERSION = -OXLAB-1207
   ```

4. 内核编译阶段，具体所用时间取决于CPU性能和配置。可以加上`-j#`参数进行多线程编译，`#`为线程数，可从`nproc`知晓。编译后的目录约1.2GB。

   ```shell
   # 编译源码
   sudo make -j1
   
   # 安装内核模块
   sudo make modules_install
   
   # 安装内核
   sudo make install
   ```

   此处我编译的是经过削减的系统，编译时间大概80分钟。

   <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\1.png" alt="1" style="zoom: 40%;" />

5. 在选择新内核启动之前，需要编辑grub2选项，开启grub菜单。

   ```shell
   sudo nano /etc/default/grub
   GRUB_TIMEOUT_STYLE=menu
   GRUB_TIMEOUT=5
   sudo update-grub
   sudo reboot
   ```

   在编辑完grub后，可以执行`grep -i timeout /boot/grub/grub.cfg`验证是否更新了`grub.cfg`。

   <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\2.png" alt="2" style="zoom:33%;" />

   

6. 在grub菜单中选择Advanced options for Ubuntu，在二级菜单中选择新编译的内核启动。（！！！由于我在编译配置的时候采用的是不完整的配置，故在重启的时候提示采用`noapic`模式启动。此时需要再次重启，在二级菜单中选中新内核，按`e`进入编辑模式，添加`noapic`，然后按下F10，可以成功启动。启动后运行`uname -a`可以查看当前内核版本号，确认是否是新内核。）

   <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\3.png" alt="3" style="zoom: 40%;" />

   <div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
       <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\4.png" alt="4" style="zoom: 30%;" />
       <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\5.png" alt="5" style="zoom: 30%;" />
   </div>
   
   
   
   <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\6.png" alt="6" style="zoom:45%;" />

### 新增`hide`系统调用

#### 1.修改`task_struct`

在`/include/linux/sched.h`中，为`task_struct`新增成员变量。规定：0表示显示，1表示隐藏。

`kernel/fork.c`的`copy_process(pid *, int, int, kernal_clone_args *)`中，为fork的子进程设置clock为0。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\7.png" alt="7" style="zoom: 38%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\8.png" alt="8" style="zoom: 38%;" />
</div>


#### 2.修改procfs

在`fs\proc\base.c`的`int proc_pid_readdir(struct file *file, struct dir_context *ctx)`以及`struct dentry *proc_pid_lookup(struct dentry *dentry, unsigned int flags)`中，增加针对`cloak`的判断语句。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\9.png" alt="9" style="zoom: 30%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\10.png" alt="10" style="zoom: 30%;" />
</div>

#### 3.添加系统调用及相关接口

**（1）新建全局变量声明文件**

在`include/linux`目录下新建`var_defs.h`头文件，声明全局隐藏开关变量`hidden_flag`，便于其他内核模块引用。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\11.png" alt="11" style="zoom: 50%;" />

**（2）实现`sys_hide`系统调用**

在`kernel`目录下新建`hide.c`文件，实现系统调用`sys_hide(pid_t pid, int on)`。该函数通过`pid_task(find_vpid(pid), PIDTYPE_PID)`查找指定进程的`task_struct`结构体（较早版本内核使用`find_task_by_pid(pid)`），并根据参数`on`的值设置进程的`cloak`字段。调用`proc_flush_pid()`函数清空VFS层缓存，确保`/proc`文件系统立即生效。权限检查通过`current_uid().val`（较早版本内核为`current->uid`）实现，仅允许root用户（uid=0）执行隐藏操作。同时使用`printk`输出内核日志便于调试。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\12.png" alt="12" style="zoom: 33%;" />

**（3）实现`sys_hide_user_processes`系统调用**

在`kernel`目录下新建`hide_user_processes.c`文件，实现系统调用`sys_hide_user_processes(uid_t uid, char *binname, int recover)`。该函数支持批量隐藏或恢复特定用户ID或进程名的所有进程，通过遍历全局进程链表`for_each_process()`实现。当`recover`为0时隐藏匹配进程，为1时恢复显示。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\13.png" alt="13" style="zoom: 27%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\14.png" alt="14" style="zoom: 29%;" />
</div>


**（4）创建`/proc/hide`控制接口**

在`fs/proc`目录下新建`hide.c`文件，提供全局隐藏功能的运行时开关。定义全局变量`hidden_flag`并通过`EXPORT_SYMBOL`导出，初始值设为1（启用隐藏）。实现`proc_read_hidden`和`proc_write_hidden`函数，支持读取和修改该开关状态。写操作中对输入字符串进行严格校验，自动去除末尾的换行符、空格等字符，避免解析错误。通过`proc_create("hide", 0644, NULL, &hide_proc_ops)`在`/proc`目录下创建文件节点，用户可通过`cat /proc/hide`读取当前状态，使用`echo "0" | sudo tee /proc/hide`动态禁用隐藏功能。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\15.png" alt="15" style="zoom: 27%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\16.png" alt="16" style="zoom: 27%;" />
</div>


**（5）创建`/proc/hidden_process`查询接口**

在`fs/proc`目录下新建`hidden_process.c`文件，提供被隐藏进程列表的查询功能。通过`for_each_process()`遍历所有进程，筛选出`cloak`字段为1的进程，将其PID拼接成空格分隔的字符串返回给用户态。用户可通过`cat /proc/hidden_process`查看当前所有被隐藏的进程ID，便于监控和调试。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\17.png" alt="17" style="zoom:33%;" />

**（6）修改编译配置文件**

在`kernel/Makefile`中的`obj-y`变量后追加`hide.o`和`hide_user_processes.o`，确保两个系统调用实现文件被静态编译进内核镜像。在`fs/proc/Makefile`中的`proc-y`变量后追加`hide.o`和`hidden_process.o`，将两个`/proc`接口文件一并编译。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\18.png" alt="18" style="zoom: 36%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\19.png" alt="19" style="zoom: 36%;" />
</div>


**（7）注册系统调用**

在`include/linux/syscalls.h`文件末尾添加两个系统调用的函数原型声明：`asmlinkage long sys_hide(pid_t pid, int on)`和`asmlinkage long sys_hide_user_processes(uid_t uid, char *binname, int recover)`。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\20.png" alt="20" style="zoom:33%;" />

在`arch/x86/entry/syscalls/syscall_64.tbl`系统调用表中添加两行记录，分别是`447  common  hide   sys_hide`以及`448  common  hide_user_processes     sys_hide_user_processes`。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\21.png" alt="21" style="zoom:50%;" />

注意插入位置应在x32系统调用段之前，避免与特殊架构调用号冲突。

### 编写测试程序及测试

重新编译安装。增量编译速度会明显快于第一次编译。推荐使用`-s`选项（`sudo make -s -j1` ），不会打印正常日志，能更容易地发现编译错误。可以重启后再观察`uname -a`是否显示为最近一次编译的结果，确定修改后的内核是否已经成功安装。

编辑`hide_test.c`和`hide_user_processes_test.c`，输入如下内容（要将宏定义的`SYSCALL_NUM`修改为之前定义过的系统调用号447和448）：

```c
// hide_test.c
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SYSCALL_NUM 447

int main()
{
    int syscallNum = SYSCALL_NUM;
    pid_t pid = 1;      // 以 PID 1 为测试对象
    int on = 1;         // 1 表示隐藏，0 表示恢复
    syscall(syscallNum, pid, on);
    return 0;
}

// hide_user_processes_test.c
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SYSCALL_NUM 448

int main()
{
    int syscallNum = SYSCALL_NUM;
    uid_t uid = 0;      // 批量隐藏 uid=0 的所有进程
    char *binname = NULL;   // 传 NULL，表示按 uid 批量处理
    int recover = 0;        // 0 表示隐藏，1 表示恢复

    syscall(syscallNum, uid, binname, recover);
    return 0;
}
```

首先使用 GNU 编译器生成两个测试程序的可执行文件，并赋予执行权限。具体命令如下：

```shell
gcc hide_test.c -o hide_test
gcc hide_user_processes_test.c -o hide_user_processes_test
chmod +x hide_test
chmod +x hide_user_processes_test
```

在完成编译之后，分别对两个系统调用进行测试，并通过 `ps`、`/proc` 以及 `dmesg` 的输出来验证隐藏效果。

**（1）`hide_test` 单进程隐藏测试**

这一部分用于验证基础的 `sys_hide(pid_t pid, int on)` 系统调用是否可以正确隐藏单个进程。测试时，我以 PID 为 1 的`init`进程（系统启动后的第一个进程）作为目标，观察其在进程列表中的可见性变化。具体步骤如下：

1. 首先使用 `ps aux | head -5` 查看系统中前若干个进程，确认此时 PID 1 的 init 进程是可见的。此时命令输出中可以清晰看到形如 `/sbin/init splash`（或 `systemd`）的记录，PID 列为 1，USER 为 root。

2. 通过 `/proc/hide` 打开全局隐藏开关。可以执行：

   ```shell
      echo "1" | sudo tee /proc/hide
      cat /proc/hide
   ```

   终端输出为 `1`，说明内核中的 `hidden_flag` 已经被设置为 1，全局隐藏功能处于启用状态。这一步同时验证了前面实现的 `/proc/hide` 写入和读取接口功能正常。

3. 在开关打开的前提下，运行 `hide_test` 调用 `sys_hide` 系统调用隐藏 PID 1 对应的进程。程序内部会调用 `syscall(447, 1, 1)`，在内核中查找到 PID 1 的 `task_struct`，将其 `cloak` 字段设置为 1，并调用 `proc_flush_pid()` 刷新 `/proc/1` 对应的缓存，使隐藏效果立即生效。

4. 再次执行 `ps aux | head -5`。此时命令输出中已经看不到原先 PID 为 1 的 init 进程记录，前几行只剩下若干内核线程（如 `kthreadd`、`rcu_gp` 等），说明在 `hidden_flag == 1` 且 `cloak == 1` 的条件下，`/proc` 进程遍历函数已经正确跳过了 PID 1。

5. 为了进一步确认系统调用确实被触发，可以执行`   dmesg | tail -n 10`查看内核日志。这些日志来自 `kernel/hide.c` 中的 `printk` 调用，侧面证明系统调用路径被正确执行，目标进程已在内核内部被标记为隐藏。

6. 执行`echo "0" | sudo tee /proc/hide`关闭开关，再次执行`ps aux | head -5`可以看到，原先被隐藏的进程重新显示出来。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\22.png" alt="22" style="zoom: 30%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\23.png" alt="23" style="zoom: 30%;" />
</div>


上述步骤可以验证：在全局开关开启的情况下，`hide_test` 能够成功隐藏指定 PID 的单个进程，`ps` 输出中不再显示该进程，全局开关关闭以后，可以重新看到被隐藏的进程。说明 `sys_hide` 系统调用及其配套的 `/proc` 过滤逻辑工作正常。

**（2）`hide_user_processes_test` 批量隐藏 root 进程测试**

第二个测试程序 `hide_user_processes_test` 用于验证扩展系统调用 `sys_hide_user_processes(uid_t uid, char *binname, int recover)` 的批量隐藏能力。这里我选择以 root 用户（uid 为 0）的所有进程为目标，重点观察“隐藏前后 root 进程数量的变化”，同时配合 `/proc/hidden_process` 的内容进行验证。测试流程如下：

1. 与前一个测试类似，首先保证全局开关处于打开状态。执行：

      ```shell
      echo "1" | sudo tee /proc/hide
      cat /proc/hide
      ```

   输出为 `1`，说明 `hidden_flag` 已经被置为启用。此时 `/proc` 层的过滤逻辑会根据 `cloak` 字段决定是否对某个进程进行隐藏。

2. 在隐藏前，先执行`ps aux | head -10`，查看当前系统中 root 用户的进程大致情况。从输出中可以看到大量 USER 列为 root 的系统服务和内核线程（包括 PID 较小的各种后台进程）。这一部分输出作为“批量隐藏前”的参考状态，用于后续对比。

3. 运行 `hide_user_processes_test`，通过系统调用一次性将 uid 为 0 的所有进程标记为隐藏。由于该操作需要 root 权限，因此使用 sudo 执行`   sudo ./hide_user_processes_test`。该程序内部调用 `syscall(448, 0, NULL, 0)`，内核中的 `sys_hide_user_processes` 会进入“按 uid 批量隐藏”的分支，对所有 `p->cred->uid.val == 0` 的进程执行如下操作：

   - 将其 `cloak` 字段设置为 1；
   - 调用 `proc_flush_pid(get_pid(p->thread_pid))` 刷新对应的 `/proc/pid` 目录项缓存；
   - 在遍历完成后输出一条内核日志 `All processes of uid 0 are hidden.`

4. 执行完测试程序后，执行`ps aux | head -10`再次查看进程列表。与隐藏前对比，可以观察到 root 用户的进程数明显减少（部分原本属于 root 的 PID 不再出现），而普通用户（如 victor）自己的桌面进程和会话进程仍然保持可见。这说明批量隐藏操作主要作用在 uid 为 0 的进程上，对其他用户的可见性没有影响。

5. 为了验证所有 root 进程确实被标记为隐藏，执行`cat /proc/hidden_process`可以查看 `/proc/hidden_process` 文件的内容，输出是一长串用空格分隔的 PID。这些 PID 对应的进程在内核中已经被设置为 `cloak == 1`，并通过 `/proc` 过滤逻辑从进程列表中被移除。可以将这一输出的截图作为“当前被隐藏 root 进程集合”的直观展示，与前面 `ps` 的结果互相印证。

6. 同样可以通过 `dmesg` 查看批量隐藏操作的内核日志。执行`dmesg | tail -n 10`可以看到类似如下的输出：

   - `[HIDE] hidden_flag changed to 1`
   - `All processes of uid 0 are hidden.`

   说明 `sys_hide_user_processes` 确实被调用，并完成了对 uid 为 0 的进程的批量处理。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\24.png" alt="24" style="zoom: 30%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_1.assets\25.png" alt="25" style="zoom: 30%;" />
</div>


综合上述两个测试程序的结果可以看到：`sys_hide` 能够精确控制单个进程的隐藏，而 `sys_hide_user_processes` 则提供了按用户 ID 维度的批量隐藏能力。再结合 `/proc/hide` 全局开关和 `/proc/hidden_process` 查询接口，可以实现对进程可见性的灵活控制。整个系统的设计具有良好的模块化特性，通过标志位而非链表操作的方式保证了内核数据结构的完整性，同时提供了丰富的运行时控制和监控手段，使得进程隐藏功能既安全可靠又便于调试和管理。测试结果表明，所有系统调用均能正常工作，进程隐藏和恢复功能符合预期，验证了设计方案的可行性和有效性。

### 实验体会

通过本次实验，我对 Linux 内核的进程管理机制有了更加深入和系统的理解。从最初对内核源码的陌生，到逐步掌握 `task_struct` 结构体的组织方式、进程链表的遍历机制以及 `/proc` 文件系统的工作原理，整个过程充满了挑战，但也收获颇丰。

在实验初期，内核编译环节就给我留下了深刻的印象。最开始没有使用 `make localmodconfig` 进行配置裁剪，而是尝试编译完整的内核，结果不仅耗时超过十个小时仍未完成，还导致虚拟机磁盘空间被完全占满。这次经历让我深刻认识到，在资源受限的环境下进行内核开发时，合理的配置管理和模块裁剪至关重要。后来通过精简配置，编译时间缩短到 80 分钟左右，磁盘占用也降低到可控范围，这种效率的提升让我意识到，了解工具链的工作机制和掌握优化技巧对于实际开发同样重要。

在系统调用的设计与实现过程中，我最大的体会是"安全性与可维护性优先"的设计原则。传统的进程隐藏方法通常直接操作 `tasks` 链表，将目标进程节点从链表中移除，这种方式虽然实现简单，但存在链表完整性破坏、恢复困难等严重问题，甚至可能导致内核崩溃。经过深入思考和资料查阅，我最终选择了通过 `cloak` 标志位结合 `/proc` 过滤逻辑的方案。这种设计不仅避免了对内核核心数据结构的破坏性修改，还提供了良好的可逆性和可观测性。在调试过程中，`/proc/hide` 和 `/proc/hidden_process` 两个接口发挥了巨大作用，前者让我可以随时开关隐藏功能，后者则帮助我快速确认当前哪些进程被标记为隐藏状态，大大降低了调试难度。

在代码实现细节上，我也遇到了不少技术难点。例如，在实现 `sys_hide_user_processes` 时，如何安全地从用户空间复制进程名字符串、如何正确遍历全局进程链表并避免竞态条件、如何通过 `proc_flush_pid()` 刷新 VFS 缓存使隐藏立即生效，这些问题都需要仔细阅读内核源码和相关博客文档才能找到正确的解决方案。特别是在处理不同内核版本的 API 差异时（如 `current_uid().val` 与 `current->uid` 的区别、`find_vpid()` 与 `find_task_by_pid()` 的差异），我深刻体会到内核开发对细节的严格要求和对版本兼容性的重视。

回顾整个实验过程，我认为最大的收获有三点：第一是对 Linux 内核模块化设计思想的深刻理解，内核通过清晰的接口划分和数据结构设计，使得功能扩展变得相对可控；第二是对系统调用机制的全面掌握，从系统调用表的注册到参数传递、权限检查、内核态与用户态的数据交互，每个环节都需要严谨的设计；第三是内核开发的实践能力得到了实质性提升，包括编译配置、代码调试、问题排查等方面的技能都有了长足进步。

此外，这次实验也让我认识到内核开发与应用层开发的巨大差异。内核代码中不能使用标准库函数，需要使用 `printk` 代替 `printf`，使用 `kmalloc` 代替 `malloc`；任何一个指针错误或内存访问越界都可能导致系统崩溃；权限控制和安全检查必须贯穿始终。这些约束虽然增加了开发难度,但也培养了我更加严谨的编程习惯和更强的代码质量意识。

总的来说，本次实验不仅让我掌握了进程管理和系统调用的核心知识，更重要的是培养了我分析问题、解决问题的能力，以及面对复杂系统时保持耐心和细心的品质。这些经验和能力的积累，对我未来在操作系统领域的深入学习和研究具有重要的指导意义。

### 参考资料

#### Linux 内核概述与进程管理

- Linux Kernel Labs - Process Management: https://linux-kernel-labs.github.io/refs/heads/master/lectures/processes.html
- The Linux Documentation Project - Process Management: https://tldp.org/LDP/tlk/kernel/processes.html
- Linux Kernel Threads and Processes (task_struct): https://cylab.be/blog/347/linux-kernel-threads-and-processes-management-task-struct
- Boston University - Linux Process Management: https://www.cs.bu.edu/fac/richwest/cs591*w1/notes/linux*process_mgt.PDF
- Linux 内核在线源码查看器: https://elixir.bootlin.com/linux/latest/source
- Linux 内核官方文档: https://www.kernel.org/doc/html/latest/

#### 内核编译与配置

- 如何快速构建精简的 Linux 内核: https://www.kernel.org/doc/html/v6.6/admin-guide/quickly-build-trimmed-linux.html
- Linux 内核编译详细教程: https://zhuanlan.zhihu.com/p/37164435
- Gentoo Wiki - 内核配置指南: https://wiki.gentoo.org/wiki/Kernel/Configuration
- Arch Linux - 传统内核编译方法: https://wiki.archlinux.org/title/Kernel/Traditional_compilation
- 使用 localmodconfig 进行非交互式配置: https://stackoverflow.com/questions/47049230/linux-kernel-build-perform-make-localmodconfig-non-interactive-way
- 内核编译默认配置最佳实践: https://unix.stackexchange.com/questions/29439/compiling-the-kernel-with-default-configurations
- 内核编译后的增量更新: https://stackoverflow.com/questions/22900073/compiling-linux-kernel-after-making-changes
- 编译时仅输出错误和警告: https://unix.stackexchange.com/questions/71154/only-output-errors-warnings-when-compile-kernel

#### 系统调用实现

- Linux 内核官方文档 - 添加系统调用: https://www.kernel.org/doc/html/latest/process/adding-syscalls.html
- 向 Linux 内核添加系统调用完整教程: https://techexplorer42.medium.com/adding-a-system-call-to-linux-kernel-d1e532b18e4e
- Linux 系统调用实现简明指南: https://www.linkedin.com/pulse/simple-guide-implementing-system-call-linux-kernel-manasi-singh-nwnzc
- Linux 系统调用执行模型深入分析: https://www.opensourceforu.com/2024/09/the-linux-system-call-execution-model-an-insight/
- Baeldung - Linux 内核系统调用实现: https://www.baeldung.com/linux/kernel-system-call-implementation
- GetVM - 编写系统调用交互式教程: https://getvm.io/tutorials/write-a-system-call
- 使用 strace 探索系统调用: https://learn.redhat.com/t5/General/Beyond-the-Veil-Exploring-System-Calls/td-p/45910
- Ubuntu 20.04 中添加系统调用: https://dev.to/jasper/adding-a-system-call-to-the-linux-kernel-5-8-1-in-ubuntu-20-04-lts-2ga8

#### procfs 文件系统

- Linux 内核官方文档 - /proc 文件系统: https://docs.kernel.org/filesystems/proc.html
- The /proc Filesystem (v5.19): https://www.kernel.org/doc/html/v5.19/filesystems/proc.html
- GitHub - proc.rst 源码文档: https://github.com/torvalds/linux/blob/master/Documentation/filesystems/proc.rst
- procfs 手册页说明: https://systutorials.com/docs/linux/man/5-procfs
- /proc 如何列举 PID 的原理: https://ops.tips/blog/how-is-proc-able-to-list-pids/
- 创建 procfs 条目（详细教程）: https://embetronicx.com/tutorials/linux/device-drivers/procfs-in-linux/#Creating*Procfs*Entry
- 创建 proc 读写条目实例: https://tuxthink.blogspot.com/2017/03/creating-proc-read-and-write-entry.html

#### 内核开发技术细节

- 新版内核中获取 UID 的方法变更: https://stackoverflow.com/questions/39229639/how-to-get-current-processs-uid-and-euid-in-linux-kernel-4-2/39230936
- 通过 PID 查找进程的替代方法: https://stackoverflow.com/questions/24447841/alternative-for-find-task-by-pid
- 内核空间访问用户空间指针: https://stackoverflow.com/questions/59772132/how-to-correctly-extract-a-string-from-a-user-space-pointer-in-kernel-space
- 导出内核符号供其他模块使用: https://www.kernel.org/doc/html/latest/kbuild/modules.html

#### 系统启动与 GRUB 配置

- Ubuntu GRUB2 启动菜单配置: https://blog.csdn.net/lu_embedded/article/details/44353499
- 显示 GRUB2 菜单的方法: https://unix.stackexchange.com/questions/373402/cant-get-grub2-menu-to-display
- 保存或导出自定义内核配置: https://superuser.com/questions/439511/how-to-save-or-export-a-custom-linux-kernel-configuration

#### 其他参考资料

- 完整但略显过时的内核教程: https://www.cnblogs.com/hellovenus/p/3967597.html
- Linux 内核学习笔记: https://www.cnblogs.com/hellovenus/p/os*linux*core_study.html
- Make 配置详解: https://www.jianshu.com/p/876043f48120