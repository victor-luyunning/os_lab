#ifndef RUN_COMMAND_H
#define RUN_COMMAND_H

#include "structures.h"

// 内置命令处理
int handle_builtin_command(char **argv, int argc, Environment *env);

// 重定向处理
int handle_redirection(char *command, Process *proc, Environment *env);

// 管道处理
int handle_pipe(char *command, Environment *env);

// 执行单个进程
int execute_process(Process *proc);

// 执行命令（主函数）
int run_command(char *line, Environment *env);

#endif
