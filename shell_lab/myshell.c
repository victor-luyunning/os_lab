#include "structures.h"
#include "utilities.h"
#include "run_command.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    Environment env;
    char line[MAX_LINE_LENGTH];
    FILE *input = stdin;
    int batch_mode = 0;
    
    // 初始化环境
    init_environment(&env);
    
    // 检查是否为批处理模式
    if (argc > 1) {
        input = fopen(argv[1], "r");
        if (!input) {
            fprintf(stderr, "Error: Cannot open file %s\n", argv[1]);
            return 1;
        }
        batch_mode = 1;
    }
    
    // 显示欢迎信息（仅交互模式）
    if (!batch_mode) {
        printf(COLOR_CYAN);
        printf("╔═══════════════════════════════════════════╗\n");
        printf("║     Welcome to Simple Shell v1.0          ║\n");
        printf("║     Type 'help' for available commands    ║\n");
        printf("║     Type 'exit' to quit                   ║\n");
        printf("╚═══════════════════════════════════════════╝\n");
        printf(COLOR_RESET);
        printf("\n");
    }
    
    // 主循环
    while (1) {
        // 打印提示符（仅交互模式）
        if (!batch_mode) {
            print_prompt(&env);
        }
        
        // 读取命令
        if (fgets(line, sizeof(line), input) == NULL) {
            break;  // EOF或错误
        }
        
        // 在批处理模式下显示执行的命令
        if (batch_mode) {
            printf("%s", line);
        }
        
        // 执行命令
        run_command(line, &env);
    }
    
    // 清理资源
    if (batch_mode) {
        fclose(input);
    }
    cleanup_environment(&env);
    
    if (!batch_mode) {
        printf("\nGoodbye!\n");
    }
    
    return 0;
}
