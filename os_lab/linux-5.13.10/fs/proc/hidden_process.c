#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

static struct proc_dir_entry *hidden_process_entry;

static ssize_t proc_read_hidden_process(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	ssize_t cnt, ret;
	char kbuf[1000];
	char tmp[16];
	struct task_struct *p;

	kbuf[0] = '\0';
	for_each_process (p) {
		if (p->cloak == 1) {
			snprintf(tmp, sizeof(tmp), "%ld ", (long)p->pid);
			strlcat(kbuf, tmp, sizeof(kbuf));
		}
	}
	cnt = strlen(kbuf);
	ret = copy_to_user(buf, kbuf, cnt);
	*ppos += cnt - ret;
	if (*ppos > cnt) return 0;
	return cnt;
}

static const struct proc_ops hidden_process_proc_ops = {
	.proc_read = proc_read_hidden_process,
};

static int __init proc_hidden_process_init(void) {
	hidden_process_entry = proc_create("hidden_process", 0444, NULL, &hidden_process_proc_ops);
	return 0;
}

static void __exit proc_hidden_process_cleanup(void)
{
	proc_remove(hidden_process_entry);
}

fs_initcall(proc_hidden_process_init);
