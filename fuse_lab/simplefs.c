#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stddef.h>

#define MAX_FILES 256
#define MAX_FILENAME 256
#define MAX_FILESIZE (1024 * 1024)  // 1MB per file

// 文件结构
typedef struct {
    char name[MAX_FILENAME];
    char *data;
    size_t size;
    int is_directory;
    time_t atime;  // 访问时间
    time_t mtime;  // 修改时间
} file_entry_t;

// 全局文件表
static file_entry_t files[MAX_FILES];
static int file_count = 0;

// 查找文件
static int find_file(const char *path) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, path) == 0) {
            return i;
        }
    }
    return -1;
}

// 获取文件属性
static int simplefs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    
    // 根目录
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    // 查找文件
    int idx = find_file(path);
    if (idx == -1) {
        return -ENOENT;
    }
    
    if (files[idx].is_directory) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = files[idx].size;
    }
    
    stbuf->st_atime = files[idx].atime;
    stbuf->st_mtime = files[idx].mtime;
    
    return 0;
}

// 读取目录
static int simplefs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    
    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }
    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    // 列出所有文件
    for (int i = 0; i < file_count; i++) {
        // 只显示根目录下的文件（去掉前导/）
        const char *name = files[i].name + 1;
        if (strchr(name, '/') == NULL) {  // 不包含子目录
            filler(buf, name, NULL, 0);
        }
    }
    
    return 0;
}

// 创建文件
static int simplefs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    if (file_count >= MAX_FILES) {
        return -ENOSPC;
    }
    
    if (find_file(path) != -1) {
        return -EEXIST;
    }
    
    // 创建新文件
    strncpy(files[file_count].name, path, MAX_FILENAME - 1);
    files[file_count].data = NULL;
    files[file_count].size = 0;
    files[file_count].is_directory = 0;
    files[file_count].atime = time(NULL);
    files[file_count].mtime = time(NULL);
    file_count++;
    
    return 0;
}

// 读取文件
static int simplefs_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    int idx = find_file(path);
    if (idx == -1) {
        return -ENOENT;
    }
    
    if (files[idx].is_directory) {
        return -EISDIR;
    }
    
    if (offset >= files[idx].size) {
        return 0;
    }
    
    if (offset + size > files[idx].size) {
        size = files[idx].size - offset;
    }
    
    if (files[idx].data != NULL) {
        memcpy(buf, files[idx].data + offset, size);
    }
    
    files[idx].atime = time(NULL);
    return size;
}

// 写入文件
static int simplefs_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) {
    int idx = find_file(path);
    if (idx == -1) {
        return -ENOENT;
    }
    
    if (files[idx].is_directory) {
        return -EISDIR;
    }
    
    size_t new_size = offset + size;
    if (new_size > MAX_FILESIZE) {
        return -EFBIG;
    }
    
    // 重新分配内存
    if (new_size > files[idx].size) {
        char *new_data = realloc(files[idx].data, new_size);
        if (new_data == NULL) {
            return -ENOMEM;
        }
        
        // 填充空白区域
        if (offset > files[idx].size) {
            memset(new_data + files[idx].size, 0, offset - files[idx].size);
        }
        
        files[idx].data = new_data;
        files[idx].size = new_size;
    }
    
    memcpy(files[idx].data + offset, buf, size);
    files[idx].mtime = time(NULL);
    
    return size;
}

// 删除文件
static int simplefs_unlink(const char *path) {
    int idx = find_file(path);
    if (idx == -1) {
        return -ENOENT;
    }
    
    if (files[idx].is_directory) {
        return -EISDIR;
    }
    
    // 释放内存
    if (files[idx].data != NULL) {
        free(files[idx].data);
    }
    
    // 移动后面的文件
    for (int i = idx; i < file_count - 1; i++) {
        files[i] = files[i + 1];
    }
    file_count--;
    
    return 0;
}

// 创建目录
static int simplefs_mkdir(const char *path, mode_t mode) {
    if (file_count >= MAX_FILES) {
        return -ENOSPC;
    }
    
    if (find_file(path) != -1) {
        return -EEXIST;
    }
    
    strncpy(files[file_count].name, path, MAX_FILENAME - 1);
    files[file_count].data = NULL;
    files[file_count].size = 0;
    files[file_count].is_directory = 1;
    files[file_count].atime = time(NULL);
    files[file_count].mtime = time(NULL);
    file_count++;
    
    return 0;
}

// 删除目录
static int simplefs_rmdir(const char *path) {
    int idx = find_file(path);
    if (idx == -1) {
        return -ENOENT;
    }
    
    if (!files[idx].is_directory) {
        return -ENOTDIR;
    }
    
    // 检查目录是否为空
    size_t path_len = strlen(path);
    for (int i = 0; i < file_count; i++) {
        if (i != idx && strncmp(files[i].name, path, path_len) == 0) {
            if (files[i].name[path_len] == '/') {
                return -ENOTEMPTY;
            }
        }
    }
    
    // 移动后面的文件
    for (int i = idx; i < file_count - 1; i++) {
        files[i] = files[i + 1];
    }
    file_count--;
    
    return 0;
}

// 截断文件
static int simplefs_truncate(const char *path, off_t size) {
    int idx = find_file(path);
    if (idx == -1) {
        return -ENOENT;
    }
    
    if (files[idx].is_directory) {
        return -EISDIR;
    }
    
    if (size > MAX_FILESIZE) {
        return -EFBIG;
    }
    
    if (size == 0) {
        if (files[idx].data != NULL) {
            free(files[idx].data);
            files[idx].data = NULL;
        }
        files[idx].size = 0;
    } else {
        char *new_data = realloc(files[idx].data, size);
        if (new_data == NULL) {
            return -ENOMEM;
        }
        
        if (size > files[idx].size) {
            memset(new_data + files[idx].size, 0, size - files[idx].size);
        }
        
        files[idx].data = new_data;
        files[idx].size = size;
    }
    
    files[idx].mtime = time(NULL);
    return 0;
}

// FUSE操作结构
static struct fuse_operations simplefs_oper = {
    .getattr    = simplefs_getattr,
    .readdir    = simplefs_readdir,
    .create     = simplefs_create,
    .read       = simplefs_read,
    .write      = simplefs_write,
    .unlink     = simplefs_unlink,
    .mkdir      = simplefs_mkdir,
    .rmdir      = simplefs_rmdir,
    .truncate   = simplefs_truncate,
};

int main(int argc, char *argv[]) {
    printf("SimpleFS - 简单内存文件系统\n");
    printf("支持功能: 创建/删除文件、读写文件、创建/删除目录\n\n");
    
    return fuse_main(argc, argv, &simplefs_oper, NULL);
}
