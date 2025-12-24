#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdbool.h>
#include <sys/types.h>

// 重定向类型枚举
typedef enum {
    NO_REDI = 0,           // 无重定向
    REDI_IN = 1,           // 输入重定向 <
    REDI_OUT = 2,          // 输出重定向 >
    REDI_BOTH = 3          // 同时有输入输出重定向
} RedirectionType;

// 进程结构体：存储单个命令的所有信息
typedef struct {
    pid_t pid;             // 进程ID
    int argc;              // 参数个数
    char **argv;           // 参数数组（包括命令本身）
    char *exec_path;       // 可执行文件的完整路径
    RedirectionType redir_type;  // 重定向类型
    int redir_infd;        // 输入重定向的文件描述符
    int redir_outfd;       // 输出重定向的文件描述符
} Process;

// Shell环境结构体：存储Shell运行环境信息
typedef struct {
    char **paths;          // PATH环境变量（查找可执行文件的路径列表）
    char cwd[1024];        // 当前工作目录
    bool path_set;         // 用户是否设置过PATH
    int path_count;        // PATH路径的数量
} Environment;

#endif
