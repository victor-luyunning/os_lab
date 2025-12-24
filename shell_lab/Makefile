# Makefile for Simple Shell

CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu99 -D_GNU_SOURCE
TARGET = myshell
OBJS = myshell.o utilities.o run_command.o

# 默认目标
all: $(TARGET)

# 链接目标文件
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# 编译源文件
myshell.o: myshell.c structures.h utilities.h run_command.h macros.h
	$(CC) $(CFLAGS) -c myshell.c

utilities.o: utilities.c utilities.h structures.h macros.h
	$(CC) $(CFLAGS) -c utilities.c

run_command.o: run_command.c run_command.h utilities.h structures.h macros.h
	$(CC) $(CFLAGS) -c run_command.c

# 清理编译文件
clean:
	rm -f $(OBJS) $(TARGET)

# 运行程序
run: $(TARGET)
	./$(TARGET)

# 使用valgrind检查内存
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

.PHONY: all clean run valgrind
