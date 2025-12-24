#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/var_defs.h>
#include <linux/module.h>

#define PROC_MAX_SIZE 16

int hidden_flag = 1;
EXPORT_SYMBOL(hidden_flag);
static struct proc_dir_entry *hide_entry;

static ssize_t proc_read_hidden(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char str[16];
	ssize_t cnt, ret;
	snprintf(str, sizeof(str), "%d\n", hidden_flag);
	cnt = strlen(str);
	ret = copy_to_user(buf, str, cnt);
	*ppos += cnt - ret;
	if (*ppos > cnt) return 0;
	return cnt;
}

static ssize_t proc_write_hidden(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char temp[PROC_MAX_SIZE];
	int tmp_flag = 0;
	size_t len;
	
	if (count > PROC_MAX_SIZE - 1) 
		count = PROC_MAX_SIZE - 1;
	
	if (copy_from_user(temp, buf, count)) 
		return -EFAULT;
	
	temp[count] = '\0';
	
	// 去除末尾的空格和换行符
	len = count;
	while (len > 0 && (temp[len-1] == '\n' || temp[len-1] == ' ' || 
	                   temp[len-1] == '\r' || temp[len-1] == '\t')) {
		temp[--len] = '\0';
	}
	
	if (len == 0)
		return -EINVAL;
	
	if (kstrtoint(temp, 10, &tmp_flag)) 
		return -EINVAL;
	
	hidden_flag = tmp_flag;
	printk(KERN_INFO "[HIDE] hidden_flag changed to %d\n", hidden_flag);
	
	return count;
}

static const struct proc_ops hide_proc_ops = {
	.proc_write = proc_write_hidden,
	.proc_read  = proc_read_hidden,
	.proc_lseek = default_llseek,
};

static int __init proc_hide_init(void)
{
	hide_entry = proc_create("hide", 0644, NULL, &hide_proc_ops);
	return 0;
}

static void __exit proc_hide_cleanup(void)
{
	proc_remove(hide_entry);
}

fs_initcall(proc_hide_init);
