# Simple Shell 实验

## 项目结构

本项目实现了一个具有管道和重定向功能的简易Shell，文件组织如下：

```
shell_lab/
├── myshell.c           # 主程序入口
├── structures.h         # 数据结构定义
├── macros.h            # 宏定义和常量
├── utilities.h/c       # 工具函数模块
├── run_command.h/c     # 命令执行模块
├── Makefile            # 编译配置
├── batch.txt           # 批处理测试脚本
└── README.md           # 本文件
```

## 编译

```bash
cd shell_lab
make
```

## 运行

### 交互模式
```bash
./myshell
```

### 批处理模式
```bash
./myshell batch.txt
```

## 支持的功能

### 1. 内置命令
- `cd [dir]` - 切换目录
- `pwd` - 显示当前目录
- `exit` - 退出Shell
- `help` - 显示帮助信息
- `path [dirs]` - 设置PATH环境变量

### 2. 外部命令
支持执行任何在PATH中的可执行程序

### 3. 管道
```bash
ls -l | grep txt | wc -l
```

### 4. 输入重定向
```bash
wc -l < input.txt
```

### 5. 输出重定向
```bash
ls > output.txt
```

### 6. 组合使用
```bash
ls | grep txt > result.txt
wc -l < input.txt > count.txt
```

### 7. 后台执行
```bash
command1 & command2
```

## 测试

运行批处理测试：
```bash
./myshell batch.txt
```

## 内存检查

使用valgrind检查内存泄漏：
```bash
make valgrind
```

## 清理

```bash
make clean
```
