CC = gcc
CFLAGS = -Wall -D_FILE_OFFSET_BITS=64
LIBS = -lfuse

TARGET = simplefs
SRC = simplefs.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "创建挂载点..."
	@mkdir -p mnt
	@echo "启动文件系统(使用 Ctrl+C 停止)..."
	./$(TARGET) -f mnt

.PHONY: all clean test
