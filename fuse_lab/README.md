# SimpleFS - 简单内存文件系统

基于FUSE实现的简单内存文件系统，支持基本的文件和目录操作。

## 功能

- ✅ 创建/删除文件
- ✅ 读写文件内容  
- ✅ 创建/删除目录

## 编译和运行

### 1. 安装依赖

```bash
sudo apt-get update
sudo apt-get install libfuse-dev
```

### 2. 编译

```bash
make
```

### 3. 运行

#### 方法一：自动运行（推荐）
```bash
make test
```

#### 方法二：手动运行
```bash
mkdir -p mnt
./simplefs -f mnt
```

### 4. 测试

打开新终端，执行以下命令：

```bash
cd mnt

# 创建文件并写入内容
echo "Hello SimpleFS" > test.txt

# 读取文件
cat test.txt

# 创建目录
mkdir mydir

# 列出文件
ls -la

# 删除文件
rm test.txt

# 删除目录
rmdir mydir
```

### 5. 卸载

回到运行文件系统的终端，按 `Ctrl+C` 停止。

或在另一个终端执行：
```bash
fusermount -u mnt
```

## 清理

```bash
make clean
rm -rf mnt
```

## 技术特点

- 纯内存存储，重启后数据消失
- 单层目录结构（暂不支持嵌套目录）
- 最多支持256个文件
- 单个文件最大1MB
