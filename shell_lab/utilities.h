#ifndef UTILITIES_H
#define UTILITIES_H

#include "structures.h"

// 环境初始化和清理
void init_environment(Environment *env);
void cleanup_environment(Environment *env);

// 字符串处理工具
char *trim_whitespace(char *str);
void parse_path(Environment *env);

// 路径查找
char *find_executable(const char *command, Environment *env);

// 进程管理工具
void init_process(Process *proc);
void cleanup_process(Process *proc);

// 打印提示符
void print_prompt(Environment *env);

#endif
