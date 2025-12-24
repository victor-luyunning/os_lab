#include "utilities.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <limits.h>

// 初始化Shell环境
void init_environment(Environment *env) {
    // 获取当前工作目录
    if (getcwd(env->cwd, sizeof(env->cwd)) == NULL) {
        strcpy(env->cwd, "/");
    }
    
    env->path_set = false;
    env->path_count = 0;
    env->paths = NULL;
    
    // 解析PATH环境变量
    parse_path(env);
}

// 清理环境资源
void cleanup_environment(Environment *env) {
    if (env->paths) {
        for (int i = 0; i < env->path_count; i++) {
            free(env->paths[i]);
        }
        free(env->paths);
    }
}

// 去除字符串首尾空白
char *trim_whitespace(char *str) {
    char *end;
    
    // 去除前导空白
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    // 去除尾部空白
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    *(end + 1) = '\0';
    return str;
}

// 解析PATH环境变量
void parse_path(Environment *env) {
    char *path_env = getenv("PATH");
    if (!path_env) {
        // 如果没有PATH环境变量，使用默认路径
        env->path_count = 2;
        env->paths = (char**)malloc(sizeof(char*) * 2);
        if (!env->paths) {
            perror("malloc");
            exit(1);
        }
        env->paths[0] = strdup("/bin");
        env->paths[1] = strdup("/usr/bin");
        return;
    }
    
    // 计算路径数量
    int count = 1;
    for (char *p = path_env; *p; p++) {
        if (*p == ':') count++;
    }
    
    // 分配内存
    env->paths = (char**)malloc(sizeof(char*) * count);
    if (!env->paths) {
        perror("malloc");
        exit(1);
    }
    env->path_count = 0;
    
    // 复制并分割PATH
    char *path_copy = strdup(path_env);
    if (!path_copy) {
        perror("strdup");
        exit(1);
    }
    
    char *token = strtok(path_copy, ":");
    while (token != NULL && env->path_count < count) {
        env->paths[env->path_count] = strdup(token);
        if (!env->paths[env->path_count]) {
            perror("strdup");
            exit(1);
        }
        env->path_count++;
        token = strtok(NULL, ":");
    }
    
    free(path_copy);
}

// 查找可执行文件
char *find_executable(const char *command, Environment *env) {
    // 如果命令包含路径分隔符，直接检查
    if (strchr(command, '/')) {
        struct stat st;
        if (stat(command, &st) == 0 && (st.st_mode & S_IXUSR)) {
            return strdup(command);
        }
        return NULL;
    }
    
    // 在PATH中查找
    for (int i = 0; i < env->path_count; i++) {
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", env->paths[i], command);
        
        struct stat st;
        if (stat(full_path, &st) == 0 && (st.st_mode & S_IXUSR)) {
            return strdup(full_path);
        }
    }
    
    return NULL;
}

// 初始化进程结构
void init_process(Process *proc) {
    proc->pid = 0;
    proc->argc = 0;
    proc->argv = NULL;
    proc->exec_path = NULL;
    proc->redir_type = NO_REDI;
    proc->redir_infd = -1;
    proc->redir_outfd = -1;
}

// 清理进程资源
void cleanup_process(Process *proc) {
    if (proc->argv) {
        for (int i = 0; i < proc->argc; i++) {
            free(proc->argv[i]);
        }
        free(proc->argv);
    }
    if (proc->exec_path) {
        free(proc->exec_path);
    }
    if (proc->redir_infd >= 0) {
        close(proc->redir_infd);
    }
    if (proc->redir_outfd >= 0) {
        close(proc->redir_outfd);
    }
}

// 打印Shell提示符
void print_prompt(Environment *env) {
    char hostname[256];
    char *username = getenv("USER");
    
    gethostname(hostname, sizeof(hostname));
    
    // 格式：username@hostname:~/path$
    printf(COLOR_GREEN "%s@%s" COLOR_RESET ":" COLOR_BLUE "%s" COLOR_RESET "$ ",
           username ? username : "user",
           hostname,
           env->cwd);
    fflush(stdout);
}
