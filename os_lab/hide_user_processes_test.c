/**

This program tests `hide_user_processes` system call.

**/

#include<stdio.h>
#include<sys/syscall.h>
#include<unistd.h>

#define SYSCALL_NUM 448

int main()
{
    int syscallNum = SYSCALL_NUM;
    uid_t uid = getuid();   // 用当前用户的 uid
    char *binname = NULL;   // 传 NULL，表示按 uid 批量隐藏
    int recover = 0;

    syscall(syscallNum, uid, binname, recover);
    return 0;
}
