# 操作系统专题实践 - 实验2

**71123329 段嘉文**

## shell的实现

[TOC]

### 实验目的

通过本次实验，深入理解操作系统中Shell命令行解释器的工作原理，掌握进程创建（fork）、进程替换（exec）、管道（pipe）、文件重定向等核心系统调用的使用方法。通过从零开始实现一个功能完备的Shell程序，提升对操作系统进程管理、进程间通信、文件系统接口等机制的理解，增强系统级编程能力和问题分析能力。

### 实验内容

1. 设计并实现一个简单的Shell命令行解释器，支持交互模式和批处理模式两种运行方式。
2. 实现基本的命令解析功能，能够正确识别命令、参数以及特殊符号（如管道、重定向符号等）。
3. 实现进程创建与执行机制，通过fork和exec系统调用族执行外部命令程序。
4. 实现输入输出重定向功能，支持标准输入重定向（<）、标准输出重定向（>）以及追加输出（>>）。
5. 实现管道功能，支持单级管道和多级管道链，实现进程间的数据流传递。
6. 实现内置命令（built-in commands），如cd（切换目录）、exit（退出Shell）、path（设置搜索路径）、help（显示帮助信息）等。
7. 编写测试脚本，验证Shell各项功能的正确性和稳定性。

### 具体要求

- Shell应支持交互模式：启动后显示提示符（如`user@host:path$`），等待用户输入命令并执行，执行完毕后继续显示提示符等待下一条命令。
- Shell应支持批处理模式：接收一个文本文件作为参数，按顺序执行文件中的每一条命令，执行完毕后自动退出。
- 实现完整的命令解析逻辑，正确处理命令、参数、空格、制表符等字符，支持任意数量的命令参数。
- 实现输入重定向（`<`）：从指定文件读取数据作为命令的标准输入。
- 实现输出重定向（`>`）：将命令的标准输出写入指定文件，若文件存在则覆盖。
- 实现追加输出（`>>`）：将命令的标准输出追加到指定文件末尾，若文件不存在则创建。
- 实现单级管道（`|`）：将前一个命令的标准输出作为后一个命令的标准输入。
- 实现多级管道链：支持多个命令通过管道串联执行（如`cmd1 | cmd2 | cmd3`）。
- 实现管道与重定向的组合使用（如`ls | grep txt > result.txt`）。
- 内置命令必须由Shell进程自身执行，不能通过fork子进程的方式实现。
- cd命令应能正确切换当前工作目录，并更新环境变量。
- path命令应能动态修改可执行文件的搜索路径。
- 实现基本的错误处理机制，对于命令不存在、文件无法打开、权限不足等错误给出明确的提示信息。
- 代码应具有良好的模块化结构，将不同功能封装成独立的函数或模块。
- 编写完整的测试用例，验证各项功能在不同场景下的正确性。

### 设计思路

本实验的设计基于Unix Shell的经典实现思路，采用模块化设计，将命令解析、进程执行、管道处理、重定向处理等功能分离成独立模块，便于开发、调试和维护。

**1. 整体架构设计**

Shell的核心工作流程可以概括为"读取-解析-执行"三个阶段：
- **读取阶段**：从标准输入或批处理文件中读取一行命令文本。
- **解析阶段**：对命令文本进行词法分析，识别命令、参数、管道符、重定向符号等元素。
- **执行阶段**：根据解析结果，通过系统调用创建进程、设置管道和重定向、执行目标程序。

为了支持交互模式和批处理模式，主程序采用统一的循环结构：检测命令来源（键盘或文件），读取命令行，调用命令执行模块，等待执行完成后继续下一轮循环。当遇到EOF（批处理文件结束或用户按Ctrl-D）或exit命令时退出循环。

**2. 环境管理策略**

定义全局的环境结构体（`Environment`），用于维护Shell的运行时状态。在Shell启动时，通过`init_environment()`函数初始化环境结构体，读取系统PATH环境变量并解析成字符串数组，同时获取当前工作目录。这种设计使得环境状态在整个Shell生命周期内保持一致，并可被各模块共享访问。

**3. 命令解析设计**

命令解析是Shell的核心模块之一，负责将用户输入的字符串转换为可执行的数据结构。解析流程分为以下几个步骤：

（1）**管道分割**：首先检测命令行中是否存在管道符`|`，若存在则按管道符分割成多个子命令，每个子命令单独解析。

（2）**重定向识别**：在每个子命令中查找`<`、`>`、`>>`符号，提取输入输出文件名，并记录重定向类型。为了正确处理`>>`追加重定向，需要检测连续的两个`>`符号。

（3）**命令与参数提取**：去除重定向部分后，剩余的字符串按空格和制表符分割，第一个词作为命令名，后续词作为参数。使用`strtok()`函数进行分词，并将结果存储在动态分配的字符串数组中。

（4）**可执行文件查找**：对于非内置命令，需要在PATH路径中查找对应的可执行文件。若命令包含`/`则视为绝对路径或相对路径直接使用，否则遍历PATH中的每个目录，拼接命令名并检查文件是否存在且可执行。

这种分层解析的方式使得每个步骤的职责明确，便于处理复杂的命令组合（如管道+重定向）。

**4. 进程执行模型**

Shell执行外部命令采用经典的fork-exec模型：
- **父进程（Shell本身）**：调用`fork()`创建子进程，然后调用`waitpid()`等待子进程执行完毕，期间保持阻塞状态。
- **子进程**：在fork后立即设置重定向（通过`dup2()`修改标准输入输出的文件描述符），然后调用`execv()`替换为目标程序。若execv失败则输出错误信息并退出。

对于内置命令（如cd、exit、path），不能通过fork子进程的方式执行，因为子进程的环境修改无法影响父进程。因此在解析阶段先判断是否为内置命令，若是则直接在当前进程中执行相应操作并返回，不进入fork-exec流程。

**5. 管道实现机制**

管道是进程间通信的重要手段，本实验采用匿名管道（`pipe()`）实现命令之间的数据流传递。对于包含n个命令的管道链，需要创建n-1个管道，每个管道包含读端和写端两个文件描述符。

管道的连接遵循以下规则：
- **第一个命令**：标准输出重定向到第一个管道的写端，标准输入保持不变（或来自输入重定向）。
- **中间命令**：标准输入来自前一个管道的读端，标准输出重定向到下一个管道的写端。
- **最后一个命令**：标准输入来自最后一个管道的读端，标准输出保持不变（或写入输出重定向）。

在所有子进程创建完毕后，父进程必须关闭所有管道的文件描述符，否则管道读端会一直等待数据导致进程阻塞。同时，父进程需要按顺序等待所有子进程结束，确保命令执行完毕。

**6. 重定向实现机制**

重定向通过修改进程的标准输入输出文件描述符实现：
- **输入重定向（`<`）**：在子进程中调用`open(filename, O_RDONLY)`打开输入文件，然后通过`dup2(fd, STDIN_FILENO)`将标准输入重定向到该文件。
- **输出重定向（`>`）**：调用`open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)`以覆盖模式打开输出文件，通过`dup2(fd, STDOUT_FILENO)`重定向标准输出。
- **追加输出（`>>`）**：使用`O_APPEND`标志代替`O_TRUNC`，使得输出内容追加到文件末尾而不是覆盖原有内容。

重定向操作必须在`execv()`之前完成，因为exec会替换整个进程空间，之后的代码不会被执行。同时，重定向可以与管道组合使用，此时需要先设置管道连接，再设置重定向，确保文件描述符的优先级正确。

**7. 错误处理与用户体验**

为了提升Shell的健壮性和用户体验，设计了以下错误处理机制：
- **命令不存在**：当在PATH中找不到可执行文件时，输出`Command not found`错误信息。
- **文件操作失败**：当打开重定向文件失败时，通过`perror()`输出系统错误信息（如权限不足、文件不存在等）。
- **系统调用失败**：对所有关键系统调用（fork、pipe、dup2、execv等）进行返回值检查，失败时输出错误并清理资源。
- **内存分配失败**：对malloc、strdup等内存分配函数进行检查，防止空指针导致段错误。

此外，Shell在交互模式下会显示彩色提示符，包含用户名、主机名、当前路径等信息，提升用户体验。在批处理模式下则不显示提示符，避免干扰输出结果。

**8. 模块化设计**

整个Shell程序按功能划分为以下模块：
- **structures.h**：定义核心数据结构（Process、Environment、RedirectionType）。
- **macros.h**：定义常量、缓冲区大小、错误消息宏。
- **utilities.h/c**：实现环境初始化、PATH解析、可执行文件查找、字符串处理等工具函数。
- **run_command.h/c**：实现命令执行的核心逻辑，包括内置命令处理、重定向处理、管道处理、进程执行等。
- **myshell.c**：主程序入口，控制交互循环和批处理流程。

这种模块化设计使得各部分职责清晰，代码易于理解和维护，同时也便于后续功能扩展（如添加新的内置命令、支持后台执行等）。

### 主要数据结构及其说明

本实验主要涉及以下核心数据结构的设计与实现：

**1. `RedirectionType` 枚举类型**

定义在`structures.h`中，用于标识进程的重定向类型：

```c
typedef enum {
    NO_REDI = 0,           // 无重定向
    REDI_IN = 1,           // 仅输入重定向 <
    REDI_OUT = 2,          // 仅输出重定向 > 或 >>
    REDI_BOTH = 3          // 同时有输入输出重定向
} RedirectionType;
```

**枚举值说明：**
- `NO_REDI`：进程不需要任何重定向，标准输入输出保持默认状态（继承自Shell）。
- `REDI_IN`：进程需要从文件读取输入，标准输入被重定向到指定文件。
- `REDI_OUT`：进程需要将输出写入文件，标准输出被重定向到指定文件。
- `REDI_BOTH`：进程同时需要输入输出重定向，如`cmd < input.txt > output.txt`。

该枚举类型简化了重定向处理逻辑，避免使用多个布尔标志位，使代码更加清晰。

**2. `Process` 结构体**

定义在`structures.h`中，描述一个待执行的进程及其相关信息：

```c
typedef struct {
    pid_t pid;                    // 进程ID（fork后由父进程记录）
    int argc;                     // 参数个数（包括命令本身）
    char **argv;                  // 参数数组，以NULL结尾
    char *exec_path;              // 可执行文件的完整路径
    RedirectionType redir_type;   // 重定向类型
    int redir_infd;               // 输入重定向的文件描述符
    int redir_outfd;              // 输出重定向的文件描述符
} Process;
```

**字段说明：**
- **pid**：进程ID，在fork成功后由父进程设置，用于后续的waitpid等待。初始化为-1表示未创建。
- **argc**：参数个数，包括命令本身。例如`ls -l /tmp`的argc为3。
- **argv**：参数数组，动态分配的字符串指针数组，第一个元素为命令名，最后一个元素为NULL（execv要求）。
- **exec_path**：可执行文件的完整路径，由`find_executable()`函数在PATH中查找得到。对于内置命令该字段为NULL。
- **redir_type**：标识该进程的重定向类型，决定是否需要设置文件描述符。
- **redir_infd**：输入重定向的文件描述符，初始化为-1表示无输入重定向。若需要重定向则通过`open()`打开文件获得。
- **redir_outfd**：输出重定向的文件描述符，初始化为-1表示无输出重定向。对于`>`使用O_TRUNC标志（覆盖），对于`>>`使用O_APPEND标志（追加）。

该结构体封装了进程执行所需的全部信息，便于在函数间传递和管理。

**3. `Environment` 结构体**

定义在`structures.h`中，维护Shell的全局运行环境：

```c
typedef struct {
    char **paths;                 // PATH环境变量解析后的目录数组
    char cwd[1024];               // 当前工作目录的绝对路径
    bool path_set;                // 用户是否通过path命令修改过PATH
    int path_count;               // PATH中的目录数量
} Environment;
```

**字段说明：**
- **paths**：动态分配的字符串指针数组，每个元素指向一个目录路径（如`"/bin"`、`"/usr/bin"`）。在Shell启动时从系统PATH环境变量解析得到，可通过path内置命令动态修改。
- **cwd**：当前工作目录的绝对路径，最大长度1024字节。由`getcwd()`获取，在cd命令执行后更新。用于提示符显示和相对路径解析。
- **path_set**：布尔标志，记录用户是否执行过path命令。若为false则使用系统默认PATH，若为true则使用用户自定义的路径列表。
- **path_count**：paths数组的有效元素个数，用于遍历PATH查找可执行文件。

该结构体在Shell启动时初始化一次，在整个生命周期内保持存在，各模块通过指针访问和修改其内容。

**4. 数据结构之间的关系**

各数据结构在Shell执行流程中的协作关系如下：

1. **Shell启动阶段**：
   - 创建`Environment`结构体，调用`init_environment()`初始化。
   - 解析系统PATH环境变量，填充`paths`数组和`path_count`。
   - 获取当前工作目录，填充`cwd`字段。

2. **命令解析阶段**：
   - 为每个子命令创建`Process`结构体，调用`init_process()`初始化所有字段。
   - 调用`handle_redirection()`解析命令、参数、重定向符号，填充`argv`、`argc`、`redir_type`、`redir_infd`、`redir_outfd`等字段。
   - 若为外部命令，调用`find_executable()`在`Environment.paths`中查找可执行文件，将结果存入`exec_path`。

3. **命令执行阶段**：
   - 若为内置命令（通过`handle_builtin_command()`判断），直接在当前进程中修改`Environment`结构体（如cd修改`cwd`，path修改`paths`）。
   - 若为外部命令，调用`fork()`创建子进程，将返回的PID存入`Process.pid`。
   - 子进程根据`redir_type`设置重定向：若`redir_infd >= 0`则调用`dup2(redir_infd, STDIN_FILENO)`，若`redir_outfd >= 0`则调用`dup2(redir_outfd, STDOUT_FILENO)`。
   - 子进程调用`execv(exec_path, argv)`执行目标程序。
   - 父进程调用`waitpid(Process.pid, NULL, 0)`等待子进程结束。

4. **管道处理阶段**：
   - 对于包含管道的命令，创建`Process`数组，每个元素对应一个子命令。
   - 创建管道数组`int pipes[n-1][2]`，通过`pipe()`初始化。
   - 每个子进程根据其在管道链中的位置，通过`dup2()`连接相应的管道读端或写端。
   - 父进程关闭所有管道文件描述符，依次等待所有子进程结束。

5. **资源清理阶段**：
   - 每个`Process`执行完毕后，调用`cleanup_process()`释放`argv`数组、`exec_path`字符串等动态分配的内存。
   - 关闭打开的文件描述符（`redir_infd`、`redir_outfd`）。
   - Shell退出前调用`cleanup_environment()`释放`Environment.paths`数组。

这种数据结构设计将Shell的状态信息和进程执行信息清晰分离，既保证了功能的完整性，又便于内存管理和错误处理。通过结构体封装，各函数的参数传递更加简洁，代码的可读性和可维护性得到显著提升。

### 编写shell部分代码

#### 1. 数据结构定义

在`structures.h`中定义了Shell程序的核心数据结构，包括重定向类型枚举`RedirectionType`、进程描述结构体`Process`以及全局环境结构体`Environment`。这些数据结构为后续的命令解析、进程管理和环境维护提供了基础支撑。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\1.png" alt="1" style="zoom:33%;" />

#### 2. 宏定义与常量

在`macros.h`中集中定义了程序中使用的各类常量，包括缓冲区大小（如`MAX_LINE_LENGTH`、`MAX_ARGS`）、路径长度限制（`MAX_PATH_LENGTH`）、管道数量上限（`MAX_PIPES`）等。同时定义了ANSI颜色代码用于美化提示符输出，以及标准化的错误消息宏（如`ERR_COMMAND_NOT_FOUND`、`ERR_FORK_FAILED`等），使得错误处理更加统一和规范。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\2.png" alt="2" style="zoom:40%;" />

#### 3. 工具函数模块实现

**（1）环境初始化函数 `init_environment()`**

在`utilities.c`中实现了Shell环境的初始化逻辑。该函数首先调用`getcwd()`获取当前工作目录并存储到`Environment.cwd`字段，然后调用`parse_path()`解析系统PATH环境变量，将其分割成独立的目录字符串数组存储在`Environment.paths`中，同时记录路径数量到`path_count`。初始化时将`path_set`标志设为false，表示使用系统默认PATH。

**（2）PATH解析函数 `parse_path()`**

该函数通过`getenv("PATH")`获取系统PATH环境变量，若不存在则设置默认路径`/bin`和`/usr/bin`。对于有效的PATH字符串，先统计冒号分隔符数量确定路径个数，然后动态分配字符串指针数组，使用`strtok()`按冒号分割PATH并逐一复制到数组中。每次内存分配后都进行错误检查，确保程序的健壮性。

**（3）可执行文件查找函数 `find_executable()`**

该函数实现了在PATH中查找可执行文件的核心逻辑。若命令名包含路径分隔符`/`，则视为绝对路径或相对路径直接检查其可执行性；否则遍历`Environment.paths`中的每个目录，拼接命令名形成完整路径，通过`stat()`检查文件是否存在且具有可执行权限（`S_IXUSR`）。找到匹配文件后返回其完整路径的副本，未找到则返回NULL。

**（4）字符串处理函数 `trim_whitespace()`**

该函数用于去除字符串首尾的空格和制表符。先从字符串开头跳过所有空白字符，再从末尾向前查找第一个非空白字符并截断，返回处理后的字符串指针。这一函数在命令解析过程中被广泛使用，确保参数提取的准确性。

**（5）提示符显示函数 `print_prompt()`**

在交互模式下，该函数负责显示彩色提示符。通过`getlogin()`获取用户名，`gethostname()`获取主机名，结合`Environment.cwd`中的当前路径，格式化输出形如`user@host:/path$`的提示符。使用ANSI颜色代码对用户名、主机名、路径进行不同颜色渲染，提升用户体验。

**（6）资源清理函数 `cleanup_environment()`**

该函数负责释放`Environment`结构体中动态分配的内存资源。遍历`paths`数组，逐一释放每个路径字符串，最后释放数组本身。在Shell退出前调用此函数，防止内存泄漏。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\3.png" alt="3" style="zoom: 30%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\4.png" alt="4" style="zoom: 30%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\5.png" alt="5" style="zoom: 30%;" />
</div>

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\6.png" alt="6" style="zoom: 30%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\7.png" alt="7" style="zoom: 30%;" />
</div>

#### 4. 命令执行模块实现

**（1）进程初始化与清理**

`init_process()`函数初始化`Process`结构体的所有字段为默认值：`pid`设为-1表示未创建，`argc`设为0，`argv`和`exec_path`设为NULL，`redir_type`设为`NO_REDI`，文件描述符设为-1。`cleanup_process()`函数释放进程相关的动态内存，包括`argv`数组中的每个字符串、数组本身以及`exec_path`，并关闭打开的重定向文件描述符。

**（2）内置命令处理函数 `handle_builtin_command()`**

该函数判断并执行Shell的内置命令。通过`strcmp()`依次检查命令是否为`exit`、`cd`、`path`或`help`：

- **exit命令**：调用`cleanup_environment()`释放资源后执行`exit(0)`退出Shell。
- **cd命令**：若无参数则切换到用户主目录（通过`getenv("HOME")`获取），有参数则切换到指定目录。调用`chdir()`执行目录切换，成功后通过`getcwd()`更新`Environment.cwd`。失败时使用`perror()`输出错误信息。
- **path命令**：先清理旧的`paths`数组，若无参数则清空PATH（`path_count`设为0），有参数则将所有参数作为新的搜索路径。将`path_set`标志设为true，表示用户已自定义PATH。
- **help命令**：输出Shell支持的功能列表和内置命令说明。

若命令匹配内置命令则返回1（表示已处理），否则返回0（交由外部命令处理流程）。

**（3）重定向处理函数 `handle_redirection()`**

该函数是命令解析的核心，负责识别和处理输入输出重定向。首先复制命令字符串以保护原始数据，然后按以下步骤解析：

1. **查找输入重定向**：在命令中搜索`<`符号，若找到则提取其后的文件名作为输入文件，调用`open()`以只读模式打开并将文件描述符存入`redir_infd`。
2. **查找输出重定向**：搜索`>`符号，检查下一个字符是否也是`>`以区分覆盖（`>`）和追加（`>>`）模式。覆盖模式使用`O_TRUNC`标志，追加模式使用`O_APPEND`标志，调用`open()`打开文件并存入`redir_outfd`。
3. **判断重定向类型**：根据输入输出文件是否存在，设置`redir_type`为`NO_REDI`、`REDI_IN`、`REDI_OUT`或`REDI_BOTH`。
4. **提取命令和参数**：去除重定向部分后，剩余字符串按空格和制表符分割，第一个词作为命令名，后续作为参数。动态分配`argv`数组并逐一复制参数字符串，数组末尾设为NULL以符合`execv()`要求。
5. **处理内置命令**：调用`handle_builtin_command()`判断是否为内置命令，若是则直接执行并返回1。
6. **查找可执行文件**：对于外部命令，调用`find_executable()`在PATH中查找完整路径，找到则存入`exec_path`，否则输出"Command not found"错误并返回-1。

**（4）进程执行函数 `execute_process()`**

该函数通过fork-exec模型执行单个进程。调用`fork()`创建子进程，父进程记录子进程PID并返回。子进程中：

1. **设置输入重定向**：若`redir_infd >= 0`，调用`dup2(redir_infd, STDIN_FILENO)`将标准输入重定向到文件，然后关闭原文件描述符。
2. **设置输出重定向**：若`redir_outfd >= 0`，调用`dup2(redir_outfd, STDOUT_FILENO)`将标准输出重定向到文件，然后关闭原文件描述符。
3. **执行目标程序**：调用`execv(exec_path, argv)`替换进程映像。若execv返回则说明执行失败，输出错误信息并调用`exit(1)`退出子进程。

**（5）管道处理函数 `handle_pipe()`**

该函数处理包含管道符的命令。首先按`|`分割命令字符串，得到多个子命令。若只有一个子命令（无管道），则直接调用`handle_redirection()`和`execute_process()`执行。对于多命令管道：

1. **创建管道**：根据命令数量创建n-1个管道，每个管道调用`pipe()`生成读写两端的文件描述符。
2. **创建子进程**：为每个子命令fork一个子进程。在子进程中：
   - **第一个命令**：将标准输出重定向到第一个管道的写端。
   - **中间命令**：将标准输入重定向到前一个管道的读端，标准输出重定向到后一个管道的写端。
   - **最后一个命令**：将标准输入重定向到最后一个管道的读端。
   - 完成重定向后，关闭所有管道文件描述符（子进程不需要其他管道），然后调用`execv()`执行命令。
3. **父进程管理**：父进程在所有子进程创建完毕后，关闭所有管道文件描述符（避免管道读端阻塞），然后按顺序调用`waitpid()`等待所有子进程结束。
4. **资源清理**：执行完毕后释放每个`Process`结构体的内存。

**（6）主命令执行函数 `run_command()`**

该函数是命令执行的入口，接收原始命令字符串和环境结构体。首先去除命令首尾空格，检查是否为空命令（直接返回）。然后检查命令中是否包含管道符`|`：若包含则调用`handle_pipe()`处理管道逻辑，否则作为单命令调用`handle_redirection()`解析后执行。整个过程中对返回值进行检查，确保错误能够正确传递。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\8.png" alt="8" style="zoom: 30%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\9.png" alt="9" style="zoom: 30%;" />
</div>

#### 5. 主程序实现

在`myshell.c`中实现了Shell的主循环控制逻辑。`main()`函数首先初始化环境结构体，然后判断运行模式：

- **批处理模式**（`argc > 1`）：将第一个参数作为批处理文件路径，调用`fopen()`打开文件。若打开失败则输出错误并退出。将输入流指向该文件，不显示提示符。
- **交互模式**（`argc == 1`）：输入流指向标准输入`stdin`，每次循环前调用`print_prompt()`显示提示符，输出欢迎信息。

主循环中，通过`fgets()`读取一行命令，若遇到EOF则退出循环。读取成功后去除末尾换行符，调用`run_command()`执行命令。循环结束后清理环境资源并返回0。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\10.png" alt="10" style="zoom: 30%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\11.png" alt="11" style="zoom: 30%;" />
</div>

#### 6. 编译配置

在`Makefile`中配置了编译规则：

- **编译器和标志**：使用`gcc`编译器，设置`-std=gnu99`和`-D_GNU_SOURCE`标志以支持`strdup()`等GNU扩展函数，启用`-Wall -Wextra -g`进行警告检查和调试。
- **目标文件**：定义了`myshell`为最终可执行文件，依赖`myshell.o`、`utilities.o`、`run_command.o`三个目标文件。
- **编译规则**：为每个源文件定义了编译规则，指定了头文件依赖关系，确保头文件修改后能触发重新编译。
- **清理规则**：提供`make clean`命令删除所有`.o`文件和可执行文件。
- **运行规则**：提供`make run`命令直接启动Shell，以及`make valgrind`命令进行内存检查。

所有代码文件编写完成后，在项目根目录执行`make`命令即可编译生成`myshell`可执行文件。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\12.png" alt="12" style="zoom:33%;" />

### 编写测试程序及测试

#### 测试环境准备

在Ubuntu虚拟机的`~/shell_lab`目录下完成代码编写后，首先执行编译命令：

```bash
cd ~/shell_lab
make clean
make
```

编译过程中，由于最初使用`-std=c99`标准导致`strdup()`和`gethostname()`函数出现隐式声明警告，后修改为`-std=gnu99`并添加`-D_GNU_SOURCE`宏定义，同时在相应源文件中包含了`<string.h>`、`<unistd.h>`、`<stdbool.h>`等必要头文件，成功消除了所有编译警告。

编译完成后生成`myshell`可执行文件，通过`ls -lh myshell`查看文件大小约为39KB。

#### 基本命令测试

**（1）交互模式启动**

执行`./myshell`启动交互模式，Shell显示欢迎信息和彩色提示符，格式为`用户名@主机名:当前路径$`。提示符中用户名显示为绿色，主机名为青色，路径为蓝色，美观清晰。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\13.png" alt="13" style="zoom: 40%;" />

**（2）简单命令执行**

依次测试以下基本命令：
- `pwd`：正确输出当前工作目录`/home/victor/shell_lab`。
- `ls`：列出当前目录下的所有文件，包括源代码、头文件、Makefile、编译生成的`.o`文件和`myshell`可执行文件。
- `ls -l`：以长格式显示文件详细信息，包括权限、所有者、大小、修改时间等，输出正确。
- `ls -a`：显示包括隐藏文件在内的所有文件。
- `echo "Hello Shell"`：正确输出字符串内容，验证了参数传递和引号处理。

所有基本命令均能正常执行，输出结果与系统Shell一致。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\14.png" alt="14" style="zoom:40%;" />

#### 内置命令测试

**（1）cd命令测试**

测试目录切换功能：
- `cd /tmp`：成功切换到`/tmp`目录，提示符中的路径更新为`/tmp`。
- `pwd`：验证当前目录确为`/tmp`。
- `cd /home/victor`：切换回用户主目录`/home/victor`。
- `cd shell_lab`：使用相对路径切换到`shell_lab`目录。
- `cd /nonexistent`：尝试切换到不存在的目录，Shell正确输出"No such file or directory"错误信息，当前目录保持不变。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\15.png" alt="15" style="zoom: 50%;" />

**（2）path命令测试**

测试PATH动态修改功能：
- 执行`path /bin /usr/bin`设置自定义PATH。
- 执行`ls`命令仍可正常运行，说明能在设定路径中找到可执行文件。
- 执行`path`（无参数）清空PATH。
- 再次执行`ls`，Shell输出"Command not found"，验证了PATH清空后无法找到命令。
- 执行`path /bin /usr/bin /usr/local/bin`恢复PATH，后续命令恢复正常。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\16.png" alt="16" style="zoom:45%;" />

**（3）help和exit命令测试**

- `help`：正确输出Shell支持的功能列表，包括内置命令说明、管道、重定向等特性介绍。
- `exit`：Shell正确退出并返回系统终端。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\17.png" alt="17" style="zoom:45%;" />

#### 输出重定向测试

**（1）覆盖重定向（`>`）测试**

- `echo "test redirect" > redirect_test.txt`：创建新文件并写入内容。
- `cat redirect_test.txt`：验证文件内容为"test redirect"。
- `echo "overwrite" > redirect_test.txt`：再次写入，覆盖原内容。
- `cat redirect_test.txt`：确认内容已被覆盖为"overwrite"。
- `ls -l > output.txt`：将ls命令的输出重定向到文件。
- `cat output.txt`：验证文件中包含完整的ls输出结果。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\18.png" alt="18" style="zoom:45%;" />

**（2）追加重定向（`>>`）测试**

- `echo "line1" > append_test.txt`：创建文件并写入第一行。
- `echo "line2" >> append_test.txt`：追加第二行。
- `echo "line3" >> append_test.txt`：追加第三行。
- `cat append_test.txt`：验证文件包含三行内容，顺序正确。
- `wc -l < append_test.txt`：使用输入重定向统计行数，输出"3"，验证追加功能正常。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\19.png" alt="19" style="zoom:50%;" />

#### 输入重定向测试

- 创建测试输入文件：`echo -e "apple\nbanana\ncherry" > fruits.txt`。
- `wc -l < fruits.txt`：从文件读取输入并统计行数，正确输出"3"。
- `sort < fruits.txt`：从文件读取并排序，输出按字母顺序排列的三个单词。
- `cat < fruits.txt`：验证输入重定向等效于直接传文件名参数。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\20.png" alt="20" style="zoom:45%;" />

#### 单级管道测试

- `ls | grep txt`：列出当前目录文件并筛选包含"txt"的文件名，正确输出所有`.txt`结尾的文件。
- `ps aux | grep bash`：查看系统进程并筛选bash相关进程，验证了对外部命令输出的管道处理。
- `echo "hello world" | wc -w`：统计单词数，输出"2"。
- `cat fruits.txt | sort`：通过管道传递文件内容进行排序。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\21.png" alt="21" style="zoom:45%;" />

所有单级管道命令均正常工作，数据流传递正确。

#### 多级管道测试

- `ls -l | grep txt | wc -l`：三级管道，先列出文件，筛选txt文件，最后统计数量，输出正确的数值。
- `cat fruits.txt | sort | head -1`：读取文件、排序、取第一行，输出""apple"。
- `ps aux | grep victor | grep bash | wc -l`：四级管道，统计当前用户的bash进程数。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\22.png" alt="22" style="zoom:50%;" />

多级管道测试验证了管道链的正确连接，每个命令的输出能够正确传递到下一个命令的输入。

#### 管道与重定向组合测试

- `ls | grep txt > result.txt`：将管道输出重定向到文件。
- `cat result.txt`：验证文件中包含筛选后的文件列表。
- `cat fruits.txt | sort > sorted.txt`：输入来自管道，输出重定向到文件。
- `wc -l < fruits.txt > count.txt`：输入输出同时重定向。
- `cat count.txt`：验证文件中包含正确的行数统计结果。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\23.png" alt="23" style="zoom:45%;" />

组合测试验证了重定向与管道可以同时使用且不会相互干扰。

#### 批处理模式测试

编写批处理测试脚本`batch.txt`，包含以下命令：

```
pwd
ls -l
echo "Hello World" > test_output.txt
cat test_output.txt
wc -l < test_output.txt
ls -l | grep test
echo "line1" > input.txt
echo "line2" >> input.txt
echo "line3" >> input.txt
wc -l < input.txt
ls -l | grep test | wc -l
ls | grep txt > result.txt
cat result.txt
rm -f test_output.txt input.txt result.txt
```

执行`./myshell batch.txt`，Shell按顺序执行文件中的每条命令，输出完整的执行结果。关键观察点：

1. 批处理模式下不显示提示符，输出更加简洁。
2. 每条命令执行后立即显示结果，与交互模式行为一致。
3. 文件创建、重定向、管道等功能均正常工作。
4. 最后的清理命令成功删除临时文件。
5. 执行完毕后Shell自动退出，返回系统终端。

<div style="display: flex; justify-content: center; gap: 10px; align-items: center; flex-wrap: nowrap;">
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\24.png" alt="24" style="zoom: 30%;" />
    <img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\25.png" alt="25" style="zoom: 30%;" />
</div>


#### 错误处理测试

**（1）命令不存在**

- 执行`nonexistentcommand`，Shell输出"nonexistentcommand: Command not found"，并继续等待下一条命令。

**（2）文件不存在**

- 执行`cat nosuchfile.txt`，系统输出"No such file or directory"错误。
- 执行`wc -l < nosuchfile.txt`，Shell在打开文件时捕获错误并输出"nosuchfile.txt: No such file or directory"。

**（3）权限不足**

- 执行`cat /etc/shadow`，系统输出"权限不够"。
- 尝试创建只读文件后写入：`touch readonly.txt; chmod 444 readonly.txt; echo "test" > readonly.txt`，Shell正确输出`Permission denied`。

**（4）空命令处理**

- 多次直接按回车，Shell正确忽略空命令，继续显示提示符等待输入。
- 输入纯空格或制表符的命令行，同样被正确识别为空命令。

<img src="E:\25-26-2\OSEXP\OSEXP\OSEXP-master\os_lab_2.assets\26.png" alt="26" style="zoom:45%;" />

通过上述全面测试，验证了Shell的各项功能均符合设计要求。

### 实验体会

通过本次Shell实验，我对操作系统中进程管理、进程间通信以及用户态与内核态交互有了更加深刻和系统的认识。从最初对Shell工作原理的模糊理解，到最终独立实现一个功能完备的命令行解释器，整个过程充满了挑战，但也收获了宝贵的经验。

在实验初期，我对fork和exec这对经典组合的理解还停留在理论层面，认为只需简单调用这两个系统调用就能执行外部命令。但在实际编码中才发现，进程创建和程序执行之间涉及大量细节：如何正确传递参数数组、如何确保数组以NULL结尾、如何在子进程中设置环境、如何避免父进程过早退出导致子进程成为孤儿进程等。特别是在实现内置命令时，我深刻体会到为什么cd、exit等命令必须由Shell自身执行——如果在子进程中修改工作目录或退出，对父进程（Shell本身）毫无影响。这一认识让我对"内置命令"与"外部命令"的本质区别有了清晰的理解。

管道的实现是本次实验中技术难度最大的部分。最初我按照网上的简单示例实现了单级管道，但在扩展到多级管道时遇到了诸多问题：管道文件描述符的管理混乱、子进程间的连接顺序错误、父进程未正确关闭管道导致读端阻塞等。经过反复调试和查阅资料，我逐渐理解了管道的本质是一个单向数据通道，必须严格遵循"写端关闭才能触发读端EOF"的规则。在实现中，我为每个管道维护了读写两端的文件描述符，在子进程中根据其在管道链中的位置（首、中、尾）设置不同的重定向策略，并在父进程中及时关闭所有管道描述符。这种精细化的资源管理让我对操作系统中"一切皆文件"的设计哲学有了更深的感悟。

重定向功能的实现相对直观，但在处理`>>`追加重定向时我犯了一个低级错误：最初只实现了`>`覆盖重定向，未考虑连续两个`>`的情况，导致测试时多次追加操作只保留最后一次写入。通过仔细分析测试输出，我发现问题出在文件打开标志上，于是修改了`handle_redirection()`函数，增加了对`>>`的专门检测逻辑，并在`open()`调用时根据重定向类型选择`O_TRUNC`（覆盖）或`O_APPEND`（追加）标志。这次调试经历让我意识到，即使是看似简单的功能，也需要考虑所有可能的情况，细节决定成败。

内存管理是本次实验中另一个需要高度重视的问题。Shell程序需要频繁进行动态内存分配：解析PATH时分配目录字符串数组、解析命令时分配参数数组、查找可执行文件时复制路径字符串等。每次分配后我都添加了错误检查代码，并在适当位置释放内存。为了确保没有内存泄漏，我使用`valgrind`工具进行了检测，发现了几处忘记释放的内存，修复后达到了"0 bytes lost"的标准。这让我深刻认识到，用户态程序虽然不像内核代码那样严格，但良好的内存管理习惯同样至关重要，它直接影响程序的稳定性和可靠性。

测试环节的工作同样重要。我编写了详细的测试用例，覆盖了基本命令、内置命令、重定向、管道、组合功能、错误处理、边界情况等多个维度。在批处理测试中，我将所有测试命令写入脚本文件，一次性执行并对比输出结果，大大提高了测试效率。测试过程中发现的问题不仅帮助我修复了代码缺陷，还加深了我对Shell各项功能的理解。例如，在测试管道时我发现某些情况下输出不完整,通过分析发现是父进程过早关闭了管道导致数据丢失，这促使我重新审视了进程同步和资源管理的逻辑。

与实验一的内核开发相比，本次用户态编程的体验截然不同。内核开发中不能使用标准库函数，必须使用内核提供的API，调试手段也相对有限，主要依赖`printk`输出日志。而用户态程序可以自由使用C标准库，可以利用`gdb`、`valgrind`等强大工具进行调试和分析，开发效率更高。但这并不意味着用户态编程更简单——Shell需要处理的逻辑复杂度、对系统调用的理解深度、对边界情况的考虑周全性，都对编程能力提出了很高要求。两次实验相辅相成，让我对操作系统的"上下两层"都有了实践性的认识。

总的来说,本次Shell实验不仅巩固了操作系统课程的理论知识,更重要的是培养了我的系统思维和工程实践能力。从需求分析到架构设计,从编码实现到测试验证,完整的开发流程让我对软件工程有了更深刻的认识。这些宝贵的经验和能力,将对我今后在操作系统领域以及其他系统级软件开发方向的学习和研究产生长远的积极影响。