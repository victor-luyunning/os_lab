#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/proc_fs.h>
#include <linux/cred.h>
#include <linux/string.h>
#include <linux/var_defs.h>

SYSCALL_DEFINE3(hide_user_processes, uid_t, uid, char *, binname, int, recover)
{
	struct task_struct *p = NULL;

	if (current_uid().val != 0) {
		printk("Permission denied. Only root can call hide_user_processes.\n");
		return 1;
	}

	if (recover == 0) {
		if (binname == NULL) {
			for_each_process (p) {
				if (p->cred->uid.val == uid && hidden_flag == 1) {
					p->cloak = 1;
					proc_flush_pid(get_pid(p->thread_pid));
				}
			}
			printk("All processes of uid %d are hidden.\n", uid);
		} else {
			char kbinname[TASK_COMM_LEN];
			long len = strncpy_from_user(kbinname, binname, TASK_COMM_LEN);
			kbinname[TASK_COMM_LEN - 1] = '\0';
			if (unlikely(len < 0)) {
				printk("Unable to do strncpy_from_user");
				return 2;
			}
			for_each_process (p) {
				char s[TASK_COMM_LEN];
				get_task_comm(s, p);
				if (p->cred->uid.val == uid &&
				    strncmp(s, kbinname, TASK_COMM_LEN) == 0 &&
				    hidden_flag == 1) {
					p->cloak = 1;
					printk("Process %s of uid %d is hidden.\n", kbinname, uid);
					proc_flush_pid(get_pid(p->thread_pid));
				}
			}
		}
	} else {
		for_each_process (p) {
			p->cloak = 0;
		}
	}
	return 0;
}
