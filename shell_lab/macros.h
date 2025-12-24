#ifndef MACROS_H
#define MACROS_H

// 缓冲区大小定义
#define MAX_LINE_LENGTH 1024      // 最大输入行长度
#define MAX_ARGS 64               // 最大参数个数
#define MAX_PROCESSES 32          // 最大并行进程数
#define MAX_PIPES 16              // 最大管道数
#define MAX_PATH_LENGTH 256       // 最大路径长度

// 颜色定义（用于提示符）
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RED     "\033[31m"

// 错误消息
#define ERR_COMMAND_NOT_FOUND "Command not found"
#define ERR_TOO_MANY_ARGS "Too many arguments"
#define ERR_PIPE_FAILED "Pipe creation failed"
#define ERR_FORK_FAILED "Fork failed"
#define ERR_FILE_OPEN_FAILED "Failed to open file"

#endif
