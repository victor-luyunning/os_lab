#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/proc_fs.h>
#include <linux/cred.h>
#include <linux/var_defs.h>

SYSCALL_DEFINE2(hide, pid_t, pid, int, on)
{
	struct task_struct *p;
	struct pid *thread_pid;
	
	printk("Syscall hide called.\n");
	p = NULL;
	if (pid > 0 && current_uid().val == 0) {
		p = pid_task(find_vpid(pid), PIDTYPE_PID);
		if (!p)
			return 1;
		if (hidden_flag == 1) {
			p->cloak = on;
			if (on == 1)
				printk("Process %d is hidden by root.\n", pid);
			if (on == 0)
				printk("Process %d is displayed by root.\n", pid);
			thread_pid = get_pid(p->thread_pid);
			proc_flush_pid(thread_pid);
		}
	} else {
		printk("Permission denied. You must be root to hide a process.\n");
	}
	return 0;
}
