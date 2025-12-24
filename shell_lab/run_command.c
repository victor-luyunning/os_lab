#include "run_command.h"
#include "utilities.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>

// 处理内置命令
int handle_builtin_command(char **argv, int argc, Environment *env) {
    if (argc == 0) return 0;
    
    // cd命令：切换目录
    if (strcmp(argv[0], "cd") == 0) {
        if (argc < 2) {
            char *home = getenv("HOME");
            if (home) {
                chdir(home);
                strcpy(env->cwd, home);
            }
        } else {
            if (chdir(argv[1]) == 0) {
                getcwd(env->cwd, sizeof(env->cwd));
            } else {
                perror("cd");
            }
        }
        return 1;
    }
    
    // exit命令：退出Shell
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }
    
    // pwd命令：显示当前目录
    if (strcmp(argv[0], "pwd") == 0) {
        printf("%s\n", env->cwd);
        return 1;
    }
    
    // help命令：显示帮助信息
    if (strcmp(argv[0], "help") == 0) {
        printf("Simple Shell - Supported commands:\n");
        printf("  cd [dir]    - Change directory\n");
        printf("  pwd         - Print working directory\n");
        printf("  exit        - Exit shell\n");
        printf("  help        - Show this help\n");
        printf("  path [dirs] - Set PATH environment\n");
        printf("\nSupported features:\n");
        printf("  Pipes:      command1 | command2\n");
        printf("  Redirect:   command < input.txt > output.txt\n");
        printf("  Background: command &\n");
        return 1;
    }
    
    // path命令：设置PATH
    if (strcmp(argv[0], "path") == 0) {
        // 清理旧的PATH
        if (env->paths) {
            for (int i = 0; i < env->path_count; i++) {
                if (env->paths[i]) free(env->paths[i]);
            }
            free(env->paths);
            env->paths = NULL;
        }
        
        if (argc == 1) {
            // 清空PATH
            env->path_count = 0;
        } else {
            // 设置新的PATH
            env->path_count = argc - 1;
            env->paths = (char**)malloc(sizeof(char*) * (argc - 1));
            if (!env->paths) {
                perror("malloc");
                return 1;
            }
            for (int i = 1; i < argc; i++) {
                env->paths[i-1] = strdup(argv[i]);
                if (!env->paths[i-1]) {
                    perror("strdup");
                    return 1;
                }
            }
        }
        env->path_set = true;
        return 1;
    }
    
    return 0;  // 不是内置命令
}

// 处理重定向
int handle_redirection(char *command, Process *proc, Environment *env) {
    char *input_file = NULL;
    char *output_file = NULL;
    bool append = false;
    char cmd_copy[MAX_LINE_LENGTH];
    strncpy(cmd_copy, command, MAX_LINE_LENGTH);
    
    // 先在整个命令中查找输入和输出重定向的位置
    char *input_redir = strchr(cmd_copy, '<');
    char *output_redir = strchr(cmd_copy, '>');
    
    // 处理输入重定向 <
    if (input_redir) {
        *input_redir = '\0';
        input_redir++;
        
        // 检查输入重定向后面是否还有输出重定向
        char *output_after_input = strchr(input_redir, '>');
        if (output_after_input) {
            *output_after_input = '\0';
            output_after_input++;
            
            // 检查是否是追加模式 >>
            if (*output_after_input == '>') {
                append = true;
                output_after_input++;
            }
            
            output_file = trim_whitespace(output_after_input);
        }
        
        input_file = trim_whitespace(input_redir);
    }
    
    // 处理输出重定向 > 或 >>
    if (output_redir && !output_file) {  // 如果还没处理过output_file
        *output_redir = '\0';
        output_redir++;
        
        // 检查是否是追加模式 >>
        if (*output_redir == '>') {
            append = true;
            output_redir++;
        }
        
        // 检查输出重定向后面是否还有输入重定向
        char *input_after_output = strchr(output_redir, '<');
        if (input_after_output) {
            *input_after_output = '\0';
            input_after_output++;
            input_file = trim_whitespace(input_after_output);
        }
        
        output_file = trim_whitespace(output_redir);
    }
    
    // 设置重定向类型
    if (input_file && output_file) {
        proc->redir_type = REDI_BOTH;
    } else if (input_file) {
        proc->redir_type = REDI_IN;
    } else if (output_file) {
        proc->redir_type = REDI_OUT;
    }
    
    // 打开输入重定向文件
    if (input_file) {
        proc->redir_infd = open(input_file, O_RDONLY);
        if (proc->redir_infd < 0) {
            perror(input_file);
            return -1;
        }
    }
    
    // 打开输出重定向文件
    if (output_file) {
        if (append) {
            proc->redir_outfd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else {
            proc->redir_outfd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        
        if (proc->redir_outfd < 0) {
            perror(output_file);
            if (input_file && proc->redir_infd >= 0) {
                close(proc->redir_infd);
            }
            return -1;
        }
    }
    
    // 解析命令和参数
    char *trimmed_cmd = trim_whitespace(cmd_copy);
    char *token = strtok(trimmed_cmd, " \t");
    
    if (!token) return -1;
    
    // 分配argv数组
    proc->argv = (char**)malloc(sizeof(char*) * MAX_ARGS);
    if (!proc->argv) {
        perror("malloc");
        return -1;
    }
    proc->argc = 0;
    
    while (token && proc->argc < MAX_ARGS - 1) {
        proc->argv[proc->argc] = strdup(token);
        if (!proc->argv[proc->argc]) {
            perror("strdup");
            return -1;
        }
        proc->argc++;
        token = strtok(NULL, " \t");
    }
    proc->argv[proc->argc] = NULL;
    
    // 处理内置命令
    if (handle_builtin_command(proc->argv, proc->argc, env)) {
        return 1;  // 内置命令已执行
    }
    
    // 查找可执行文件
    proc->exec_path = find_executable(proc->argv[0], env);
    if (!proc->exec_path) {
        fprintf(stderr, "%s: %s\n", proc->argv[0], ERR_COMMAND_NOT_FOUND);
        return -1;
    }
    
    return 0;
}

// 执行单个进程
int execute_process(Process *proc) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror(ERR_FORK_FAILED);
        return -1;
    }
    
    if (pid == 0) {
        // 子进程
        
        // 设置输入重定向
        if (proc->redir_infd >= 0) {
            dup2(proc->redir_infd, STDIN_FILENO);
            close(proc->redir_infd);
        }
        
        // 设置输出重定向
        if (proc->redir_outfd >= 0) {
            dup2(proc->redir_outfd, STDOUT_FILENO);
            close(proc->redir_outfd);
        }
        
        // 执行命令
        execv(proc->exec_path, proc->argv);
        
        // 如果execv返回，说明出错
        perror(proc->exec_path);
        exit(1);
    }
    
    // 父进程
    proc->pid = pid;
    return 0;
}

// 处理管道
int handle_pipe(char *command, Environment *env) {
    char *commands[MAX_PIPES];
    int cmd_count = 0;
    
    // 分割管道命令
    char *token = strtok(command, "|");
    while (token && cmd_count < MAX_PIPES) {
        commands[cmd_count] = trim_whitespace(token);
        cmd_count++;
        token = strtok(NULL, "|");
    }
    
    if (cmd_count == 1) {
        // 没有管道，直接处理重定向
        Process proc;
        init_process(&proc);
        
        int ret = handle_redirection(commands[0], &proc, env);
        if (ret == 1) {
            // 内置命令已执行
            cleanup_process(&proc);
            return 0;
        }
        if (ret < 0) {
            cleanup_process(&proc);
            return -1;
        }
        
        execute_process(&proc);
        waitpid(proc.pid, NULL, 0);
        cleanup_process(&proc);
        return 0;
    }
    
    // 处理管道
    int pipes[MAX_PIPES][2];
    Process processes[MAX_PIPES];
    
    // 创建所有管道
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror(ERR_PIPE_FAILED);
            return -1;
        }
    }
    
    // 创建所有进程
    for (int i = 0; i < cmd_count; i++) {
        init_process(&processes[i]);
        
        int ret = handle_redirection(commands[i], &processes[i], env);
        if (ret < 0) {
            // 错误处理
            for (int j = 0; j <= i; j++) {
                cleanup_process(&processes[j]);
            }
            return -1;
        }
        if (ret == 1) {
            // 内置命令不能在管道中使用
            fprintf(stderr, "Builtin commands cannot be used in pipes\n");
            for (int j = 0; j <= i; j++) {
                cleanup_process(&processes[j]);
            }
            return -1;
        }
        
        pid_t pid = fork();
        if (pid < 0) {
            perror(ERR_FORK_FAILED);
            return -1;
        }
        
        if (pid == 0) {
            // 子进程
            
            // 第一个命令：只连接输出到管道
            if (i == 0) {
                if (processes[i].redir_infd < 0) {
                    // 没有输入重定向，使用标准输入
                }
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            // 最后一个命令：只连接输入从管道
            else if (i == cmd_count - 1) {
                dup2(pipes[i-1][0], STDIN_FILENO);
                if (processes[i].redir_outfd < 0) {
                    // 没有输出重定向，使用标准输出
                }
            }
            // 中间命令：输入和输出都连接管道
            else {
                dup2(pipes[i-1][0], STDIN_FILENO);
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // 关闭所有管道
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // 处理重定向
            if (processes[i].redir_infd >= 0) {
                dup2(processes[i].redir_infd, STDIN_FILENO);
                close(processes[i].redir_infd);
            }
            if (processes[i].redir_outfd >= 0) {
                dup2(processes[i].redir_outfd, STDOUT_FILENO);
                close(processes[i].redir_outfd);
            }
            
            // 执行命令
            execv(processes[i].exec_path, processes[i].argv);
            perror(processes[i].exec_path);
            exit(1);
        }
        
        processes[i].pid = pid;
    }
    
    // 父进程关闭所有管道
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // 等待所有子进程
    for (int i = 0; i < cmd_count; i++) {
        waitpid(processes[i].pid, NULL, 0);
        cleanup_process(&processes[i]);
    }
    
    return 0;
}

// 执行命令（主函数）
int run_command(char *line, Environment *env) {
    if (!line || strlen(line) == 0) {
        return 0;
    }
    
    // 去除换行符
    char *newline = strchr(line, '\n');
    if (newline) *newline = '\0';
    
    // 去除首尾空白
    line = trim_whitespace(line);
    if (strlen(line) == 0) {
        return 0;
    }
    
    // 处理后台运行 &
    char *commands[MAX_PROCESSES];
    int cmd_count = 0;
    
    char *token = strtok(line, "&");
    while (token && cmd_count < MAX_PROCESSES) {
        commands[cmd_count] = trim_whitespace(token);
        if (strlen(commands[cmd_count]) > 0) {
            cmd_count++;
        }
        token = strtok(NULL, "&");
    }
    
    // 执行所有命令
    for (int i = 0; i < cmd_count; i++) {
        handle_pipe(commands[i], env);
    }
    
    return 0;
}
