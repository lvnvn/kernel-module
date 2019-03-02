#include <linux/module.h> // module_init, ...
#include <linux/kernel.h> // printk
#include <linux/init.h> //__init, __exit
#include <linux/proc_fs.h>
#include <linux/uaccess.h> 
#include <linux/fs_struct.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/fdtable.h>
#include <linux/vmalloc.h>
#include "linux/sched/task.h"

#define MAX_WRITE_BUF_SIZE 10
#define DEPTH 1000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lvnvn");
MODULE_DESCRIPTION("kernel module");

typedef struct PList
{
    struct task_struct* task;
    struct PList* next;
}PList;

int str_to_number(const char *str, int n);
void mem_info(int id, char* buf);
void files_info(int id, char* buf);
void process_info(struct task_struct* task, int n, char* buf, int* offset);

static char *msg_tree;
static char *module_command;

// send message to user-space
ssize_t read_proc_file(struct file *filp,char *buf, size_t count,loff_t *offp) 
{
	char *buf_msg = vmalloc(100000 * sizeof(char));
	if(strncmp(module_command,"tree",4)==0)
	{
		struct task_struct *task;
		task = &init_task;
		int offset = 0;
		process_info(task, DEPTH, buf_msg, &offset);
	}
	else if(strncmp(module_command,"mem",3)==0 || strncmp(module_command,"fls",3)==0)
	{
		// get pid from module_command
		int idx = 3;
		while(module_command[idx+1] != '\0')
			idx++;
		char *pid_c = vmalloc(idx-2 * sizeof(char));
		memcpy(pid_c, &module_command[3], idx-2);
		int pid = str_to_number(pid_c,idx-2);
		
		if(strncmp(module_command,"mem",3)==0) 
			mem_info(pid, buf_msg);
		if(strncmp(module_command,"fls",3)==0)
			files_info(pid, buf_msg); 
	}
	
	// read() shall start at a position in the file given by offp (file offset)
	int res;
	if(*offp >= strlen(buf_msg))
	{    
		*offp = 0;
		return 0;
	}
    if(count > strlen(buf_msg) - *offp) 
       count = strlen(buf_msg) - *offp;
    res = copy_to_user((void*)buf, buf_msg + *offp, count);
    *offp += count; // offp shall be incremented by the number of bytes actually read
    return count;
}

// get messages from user-space
ssize_t write_proc_file(struct file *filp,const char *buf, size_t count,loff_t *offp)
{
	ssize_t procfs_buf_size = count;
	if (procfs_buf_size > MAX_WRITE_BUF_SIZE)
	{
		procfs_buf_size = MAX_WRITE_BUF_SIZE;
	}
	copy_from_user(module_command, buf, procfs_buf_size);

	module_command[procfs_buf_size] = '\0'; 
	printk(KERN_INFO "+ Message to /proc/%s from user-space: '%s'\n", "module_proc_file", module_command);

	return count;
}

// Init-функция, вызываемая при загрузке модуля 
static int __init my_module_init(void) {
	// proc_dir_entry - struct with information about /proc file
	static struct proc_dir_entry *procfs_tree;
	// reading and writing callbacks
	static const struct file_operations proc_file_fops_tree = {
		.owner = THIS_MODULE,
	 	.write = write_proc_file,
	 	.read = read_proc_file,
	};
	// create proc file (returns non-zero if the file is a regular file; параметр может читаться кем угодно, но не может быть изменён; )
	procfs_tree = proc_create("module_proc_file", S_IFREG | S_IRUGO | S_IWUGO, NULL, &proc_file_fops_tree);
	module_command = kmalloc(MAX_WRITE_BUF_SIZE * sizeof(char), GFP_KERNEL);
	if(procfs_tree==NULL) // proc file wasn't created
	{
		remove_proc_entry("module_proc_file", NULL);
		printk("+ Error: Could not initialize files in /proc\n");
		return -ENOMEM;
	}
	printk(KERN_INFO "+ Module is now loaded.\n");
	return 0;
}

// Cleanup-функция, вызываемая при выгрузке модуля 
static void __exit my_module_cleanup(void) {
	kfree(module_command);
	remove_proc_entry("module_proc_file", NULL);
	printk(KERN_INFO "+ Module is now unloaded.\n");
}

// Ядру сообщаются названия функций, вызываемых при загрузке и выгрузке модуля
module_init(my_module_init);
module_exit(my_module_cleanup);

// обход дерева процессов и запись в файл buf_msg
void process_info(struct task_struct* task, int n, char* buf, int* offset)
{
	char *str = vmalloc(1000 * sizeof(char));
	int cnt;
    
	int count = 0;
    struct PList* head = kmalloc(sizeof(PList), GFP_KERNEL);
    head -> task = NULL;
    head -> next = NULL;
    struct PList* cur = head;
    struct list_head* pos;
    list_for_each(pos, &task->children)
    {
        if (head -> task == NULL)
            head -> task = list_entry(pos, struct task_struct, sibling);
        else
        {
            cur -> next = kmalloc(sizeof(PList),GFP_KERNEL);
            cur -> next -> task = list_entry(pos, struct task_struct, sibling);
            cur -> next -> next = NULL;
            cur = cur -> next;
        }
        count++;
    }
    // comm - имя исполняемого файла процесса
	cnt = sprintf(str, "Process: %s[%d], Parent: %s[%d]\n", task -> comm, task -> pid , task -> parent -> comm, task -> parent -> pid);
	str[cnt] = '\0';
	memcpy(buf + (*offset), str, strlen(str));
	(*offset) += strlen(str);
    if(count > 0)
    {
        struct PList* pr;
        n = n - 1;
        int i = 1;
        if(n > 0)
		for(pr = head; pr != NULL;)
        {
        	int m = DEPTH;
            for(; m > n; m--)
            {
				memcpy(buf + (*offset), " ", strlen(" "));
				(*offset) += strlen(" ");
            }
			cnt = sprintf(str, "%d) ", i); // number of child
			str[cnt] = '\0';
			memcpy(buf + (*offset), str, strlen(str));
			(*offset) += strlen(str);
            process_info(pr -> task, n, buf, offset);
            i = i+1;
            struct PList* temp = pr;
            pr = pr -> next;
            kfree(temp);
            temp = NULL;
        }
    }
	
	buf[*offset] = '\0';
	vfree(str);	  
	return 0;
}

void mem_info(int id, char* buf)
{
	char *str = vmalloc(1000 * sizeof(char));
	int offset = 0;
	int cnt;

	struct task_struct *task;
	struct mm_struct *m; // данные об используемой процессом памяти
	struct vm_area_struct *v; // абстрактная структура для работы со страницами
	struct kstat *ks; //  kernel statistics structure
	unsigned long t_size = 0;

	task = pid_task(find_vpid(id), PIDTYPE_PID);

	if(task != NULL)
	{
		cnt = sprintf(str, "Process: %s[%d]\n",task -> comm, task -> pid);
		str[cnt] = '\0';
		memcpy(buf + offset, str, strlen(str));
		offset += strlen(str);

		struct mm_struct *v0 = task -> mm; // mm - memory descriptor; 
		struct vm_area_struct *v = NULL; // mmap - memory mapping segment
		if(v0 != NULL)
			v = v0 -> mmap;
		if(v != NULL)
		{
			while(v -> vm_next != NULL)
			{
				unsigned long size = v -> vm_end - v -> vm_start;
				t_size = t_size + size;
				cnt = sprintf(str, "Start: 0x%lx, End: 0x%lx, Block Size: 0x%lx\n", v -> vm_start, v -> vm_end, size);
				str[cnt] = '\0';
				memcpy(buf + offset, str, strlen(str));
				offset += strlen(str);
				v = v -> vm_next;
			}
			cnt = sprintf(str, "Total size of virtual space is: 0x%lx\n",t_size);
			str[cnt] = '\0';
			memcpy(buf + offset, str, strlen(str));
			offset += strlen(str);
		}
	}
	else
	{
		cnt = sprintf(str, "ID %d not found\n", id);
		str[cnt] = '\0';
		memcpy(buf + offset, str, strlen(str));
		offset += strlen(str);
	}	
	buf[offset] = '\0';
	vfree(str);
}

void files_info(int id, char* buf)
{
	char *str = vmalloc(1000 * sizeof(char));
	int offset = 0;
	int cnt;

	struct task_struct *task;
	struct files_struct *open_files;
	struct fdtable *files_table; 
	struct path files_path;

	task = pid_task(find_vpid(id), PIDTYPE_PID);

	if(task != NULL)
	{
		cnt = sprintf(str, "Process: %s[%d]\n",task -> comm, task -> pid);
		str[cnt] = '\0';
		memcpy(buf + offset, str, strlen(str));
		offset += strlen(str);

		int i = 0;
		open_files = task -> files;
		files_table = files_fdtable(open_files);
		char *path;
		char *buf_tmp = (char*)kmalloc(10000 * sizeof(char), GFP_KERNEL);
		while(files_table -> fd[i] != NULL)
		{
			files_path = files_table -> fd[i] -> f_path;
			char* name = files_table-> fd[i] -> f_path.dentry -> d_iname;
			long long size = i_size_read(files_table-> fd[i] -> f_path.dentry -> d_inode);
			path = d_path(&files_path, buf_tmp, 10000 * sizeof(char));
			cnt = sprintf(str, "Name: %s, FD: %d, Size: 0x%llx bytes, Path: %s\n", name, i, size , path);
			str[cnt] = '\0';
			memcpy(buf + offset, str, strlen(str));
			offset += strlen(str);
			i++;
		}
		kfree(buf_tmp);
	}
	else
	{
		cnt = sprintf(str, "ID %d not found\n", id);
		str[cnt] = '\0';
		memcpy(buf + offset, str, strlen(str));
		offset += strlen(str);
	}
	
	buf[offset] = '\0';
	vfree(str);
}

int str_to_number(const char *str, int n)
{
	int res = 0;
	int i;
	for(i = 0; i < n; i++)
		res = res * 10 + ( str[i] - '0' );
	return res;
}