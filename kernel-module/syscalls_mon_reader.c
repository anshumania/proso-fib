/*
 * syscalls_mon_reader.c
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include "include/syscalls_mon.h"
#include "include/syscalls_mon_reader.h"

MODULE_DESCRIPTION ("PROSO Driver - Statistics reader");
MODULE_LICENSE ("GPL");


static int __init syscalls_mon_driver_init(void){
	int result;

	proc_monitored = 0;
	sysc_monitored = 0;
	open_pid = 1;

	/* The device is reserved */
	dev_id = MKDEV (MAJ, MIN);
	result = register_chrdev_region (dev_id, 1, "syscalls_stats_reader");
	if (result < 0){
		printk ("ERROR: register_chrdev_region");
		return result;
	}

	/* The the device is allocated and created */
	my_dev = cdev_alloc ();
	if (my_dev == NULL){
		printk ("ERROR: cdev_alloc");
		return -1;
	}

	/* We set the properties of the device */
	my_dev->owner = THIS_MODULE; /* Using this macro, the management of references to the module will be handled by the system by its own. */
	my_dev->ops = &file_ops; /* Associates specific file_operations to our device. */
	result = cdev_add (my_dev, dev_id, 1);
	if (result < 0){
		printk ("ERROR: cdev_add");
		return result;
	}

	// Makes the device available
	lock = 0;

	printk (KERN_DEBUG "Syscalls Reader Driver successfully loaded.\n");
	return 0;
}

static void __exit syscalls_mon_driver_exit(void){
	/* Releases the identifiers of driver */
	unregister_chrdev_region (dev_id, 1);
	/* Deletes the driver structure */
	cdev_del (my_dev);

	printk (KERN_DEBUG "Syscalls Reader Driver succesfully unloaded.\n");
}
module_init(syscalls_mon_driver_init);
module_exit(syscalls_mon_driver_exit);

/* This call returns to the user (through *buffer) a structure containing informations
	 * about the syscall currently monitored.
	 * The number of bytes read will be the minimum between the requested size and the size of
	 * the structure .
	 */
ssize_t readstats_read_dev (struct file *f, char /*__user*/ * buffer, size_t s, loff_t * off){
	struct t_info info;
	size_t sz;
	int result;

	result = get_stats (proc_monitored, sysc_monitored, &info);
	if (result < 0)
		return result;

	if (s < 0)
		return -EINVAL;

	(s < sizeof(struct t_info))? (sz=s) : (sz=sizeof(struct t_info));

	return (ssize_t) copy_to_user (buffer, &info, sz);
}

/* This function allows to change the behavior of the device.
 * */
int readstats_ioctl_dev (struct inode *i, struct file *f, unsigned int arg1, unsigned long arg2){
	int result, argument2;
	struct task_struct *task;
	struct my_thread *thinfo_stats;

	argument2=(signed int) arg2;

	/* Depending on the first parameter, different functions will be
	 * selected. */
	switch (arg1){
		case 0:
			/* CHANGE_PROCESS (arg1=0): in this case arg2 indicates, by reference, the PID
			 * of the process we want to analyze.
			 * If arg2 == NULL: the PID used for the opening of the device will be used.
			 * If no process with PID==arg2 exists, an error will be returned.
			 */
			if (argument2 < 0)
				return -EINVAL;

			/* We search for the process and, if it doesn't exist, an error is returned */
			task = find_task_by_pid ((pid_t) argument2);
			if (task == NULL){
				// Process to be monitored will be the one used during the opening of the device
				proc_monitored = open_pid;
				return -ESRCH;
			}
			else{
				/* Obtains statistics structure of the process */
				thinfo_stats = (struct my_thread *) task->thread_info;

				/* Controls for consistency in statistics of process */
				if (argument2 != thinfo_stats->pid)
					// if they are not consistent, we reset them
					reset_stats ((pid_t) argument2, thinfo_stats);

				// Sets the new process to be monitored
				proc_monitored = argument2;
			}
			break;

		case 1:
			/* CHANGE_SYSCALL (arg1=1): this changes the syscall monitored and read by readstats_read_dev
			 * arg2 indicates which syscall will be monitored:
			 * OPEN ->  0
			 * CLOSE -> 1
			 * WRITE -> 2
			 * CLONE -> 3
			 * LSEEK -> 4 */

			if (argument2 < 0 || argument2 >= N_CALLS)
				return -EINVAL;

			// Sets the new syscall to be monitored
			sysc_monitored = argument2;
			break;

		case 2:
			/* RESET_VALUES (arg1=2): resets the statistics of the process monitored */
			result = reset_values (proc_monitored);
			break;

		case 3:
			/* RESET_VALUES_ALL_PROCESSES (arg1=3): resets the statistics of every process */
			complete_reset ();
			break;

		case 4:
			/* ENABLE_SYS_CALL (arg1=4): enables the monitoring for the syscall indicated by arg2.
			 * If arg2 < 0, all the syscalls will be activated.*/
			if (argument2 >= N_CALLS)
				return -EINVAL;

			activate_syscall_mon (argument2);
			break;

		case 5:
			/* DISABLE SYS_CALL (arg1=5): disables the monitoring for the syscall indicated by arg2.
			 * If arg2 < 0, all the syscalls will be disabled.*/
			if (argument2 >= N_CALLS)
				return -EINVAL;

			deactivate_syscall_mon (argument2);
			break;

		default:
			/* Unknown parameters */
			return -EINVAL;
			break;
		}
	return 0;
}

int readstats_open_dev (struct inode *i, struct file *f){
	  // If the device is already open, return error
	  if (lock != 0)
	    return -EPERM;

	  // If the process accessing the device doesn't have ROOT privileges, return error
	  if (current->uid != 0)
	    return -EACCES;

	  lock++; // Lock the device
	  open_pid = current->pid; // Saves the pid of the process which opened the device
	  proc_monitored=open_pid;
	  return 0;
}

int readstats_release_dev (struct inode *i, struct file *f){
	  // If the device is not locked (meaning it has not been opened or it's already closed), return error
	  if (!lock)
	    return -EPERM;

	  lock--; // Release the device
	  open_pid = 0;
	  return 0;
}

/* This function allows to reset the statistics of a specified process (identified by pid) */
int reset_values (int pid){
	  struct task_struct *t;

	  if (pid < 0)
	    return -EINVAL;

	  /* Searches for the process identified by pid */
	  t = find_task_by_pid (pid);
	  if (t < 0)
	    return -ESRCH;

	  /* Resets process stats */
	  reset_stats (pid, (struct my_thread *) t->thread_info);
	  return 0;
}

/* This function simply iterates reset_stats() for every process */
void complete_reset (void){
	  struct task_struct *t;
	  //printk(KERN_DEBUG "MSG: sono dentro a complete_reset()\n");
	  for_each_process (t)
		  reset_stats (t->pid, (struct my_thread *) t->thread_info);
}

/* activates the monitor over the specified syscall */
int activate_syscall_mon (int call_index){
	  int i;
	  if (call_index >= N_CALLS)
	    return -EINVAL;

	  //printk(KERN_DEBUG "MSG: sono dentro a activate_syscall_mon() e call_index=%i\n",call_index);

	  if (call_index > -1) // activates one single syscall
		  set_mon_on_syscall (call_index);
	  else // activates all the syscalls
	    for (i = 0; i < N_CALLS; i++)
	    	set_mon_on_syscall (i);
	  return 0;
}

/* disables the monitor over the specified syscall */
int deactivate_syscall_mon (int call_index){
	  int i;
	  if (call_index >= N_CALLS)
	    return -EINVAL;

	  //printk(KERN_DEBUG "MSG: sono dentro a deactivate_syscall_mon() e call_index=%i\n",call_index);

	  if (call_index > -1) // disables one single syscall
		  unset_mon_on_syscall (call_index);
	  else // disables all the syscalls
	    for (i = 0; i < N_CALLS; i++)
	    	unset_mon_on_syscall (i);
	  return 0;
}
