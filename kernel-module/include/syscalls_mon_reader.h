/*
 * syscalls_mon_reader.h
 *
 */

#ifndef SYSCALLS_MON_READER_H_
#define SYSCALLS_MON_READER_H_

#define MAJ 253
#define MIN 0

/* This variable will be used to assure that the device can be accessed by one process at time.
 * lock = 1 -> busy
 * lock = 0 -> free
 **/
short int lock;

int open_pid;
int proc_monitored;
int sysc_monitored;

dev_t dev_id;
struct cdev *my_dev;

ssize_t readstats_read_dev (struct file *f, char /*__user*/ * buffer, size_t s, loff_t * off);
int readstats_ioctl_dev (struct inode *i, struct file *f, unsigned int arg1,
                     unsigned long arg2);
int readstats_open_dev (struct inode *i, struct file *f);
int readstats_release_dev (struct inode *i, struct file *f);

int reset_values (int pid);
void complete_reset (void);
int activate_syscall_mon (int call_index);
int deactivate_syscall_mon (int call_index);

//unsigned long copy_to_user (void *to, const void *from, unsigned long count);

struct file_operations file_ops = {
	owner:THIS_MODULE,
	read:readstats_read_dev,
	ioctl:readstats_ioctl_dev,
	open:readstats_open_dev,
	release:readstats_release_dev,
};

#endif /* SYSCALLS_MON_READER_H_ */
