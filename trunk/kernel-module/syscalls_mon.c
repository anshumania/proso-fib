/*
 * syscalls_mon.c
 *
 * Dumb syscalls monitor.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/signal.h>
//#include <sys/syscall.h>
#include "include/syscalls_mon.h"

pid_t pid_init=1;
module_param(pid_init, int, 0);
MODULE_PARM_DESC(pid_init,"Process ID to monitor (default 1)");
MODULE_DESCRIPTION("PROSO Project 2: module 2, reader of process statistics");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(reset_stats);
EXPORT_SYMBOL(get_stats);
EXPORT_SYMBOL(unset_mon_on_syscall);
EXPORT_SYMBOL(set_mon_on_syscall);
EXPORT_SYMBOL(print_stats);

/* Initialization code */
static int __init syscalls_mon_init(void){
	struct task_struct *t;

	/* We have to save the original addresses of each syscall we want to monitor */
	sys_call_table_original[OPEN] = sys_call_table[__NR_open];
	sys_call_table_original[CLOSE] = sys_call_table[__NR_close];
	sys_call_table_original[MYWRITE] = sys_call_table[__NR_write];
	sys_call_table_original[CLONE] = sys_call_table[__NR_clone];
	sys_call_table_original[LSEEK] = sys_call_table[__NR_lseek];

	/* We save the addresses of the local traps into a local table */
	sys_call_table_local[OPEN]  = sys_open_local;
	sys_call_table_local[CLOSE] = sys_close_local;
	sys_call_table_local[MYWRITE] = sys_write_local;
	sys_call_table_local[CLONE] = sys_clone_local;
	sys_call_table_local[LSEEK] = sys_lseek_local;

	/* Here we update the addresses of local traps into the sys_call_table.
	 * By default, when the module is loaded, the 5 syscalls will be monitored. */
	sys_call_table[__NR_open]  = sys_call_table_local[OPEN];
	sys_call_table[__NR_close] = sys_call_table_local[CLOSE];
	sys_call_table[__NR_write] = sys_call_table_local[MYWRITE];
	sys_call_table[__NR_clone] = sys_call_table_local[CLONE];
	sys_call_table[__NR_lseek] = sys_call_table_local[LSEEK];

	/* We search for the process identified by the pid inserted as parameter*/
	t = find_task_by_pid (pid_init);

	/* If it doesn't exist, then we take pid=1 by default */
	if (t == NULL){
		pid_init = 1;
		t = find_task_by_pid (pid_init);
		printk (KERN_DEBUG "No such process for the specified PID...I'll use default value (1).\n");
	}

	/* We copy the parameter to make it accessible from outside the module
	pid = pid_init;*/

	/* Reinitializes statistics for the selected process*/
	reset_stats (pid_init, (struct my_thread *) t->thread_info);

	printk(KERN_DEBUG "SysCalls Monitor successfully loaded on PID=%i\n",pid_init);

	return 0;
}

/* Unload the module */
static void __exit syscalls_mon_exit(void){
	struct task_struct *t;

	/* Here the original values of the sys_call_table are restored */
	sys_call_table[__NR_open] = sys_call_table_original[OPEN];
	sys_call_table[__NR_close] = sys_call_table_original[CLOSE];
	sys_call_table[__NR_write] = sys_call_table_original[MYWRITE];
	sys_call_table[__NR_clone] = sys_call_table_original[CLONE];
	sys_call_table[__NR_lseek] = sys_call_table_original[LSEEK];

	t = find_task_by_pid (pid_init);

	if (t == NULL){
		printk (KERN_DEBUG "PID %i doesn't exist anymore. No statistics will be printed.\n",pid_init);
	}
	else{
		printk(KERN_DEBUG "Statistics for specified process:\n");
		print_stats (pid_init); /* We print statistics of pid_init if it still exists */

		printk(KERN_DEBUG "\n\nGLOBAL Statistics:\n");
		print_total_sys_stats();
	}

	printk (KERN_DEBUG "Syscalls Monitor unloaded.\n");
}
module_init(syscalls_mon_init);
module_exit(syscalls_mon_exit);

long sys_open_local (const char /*__user*/ * filename, int flags, int mode){

	long (*syscall) (const char /*__user*/ * filename, int flags, int mode);
	unsigned long long start, end;
	struct my_thread *thinfo_stats;
	struct t_info *pidstats;
	int result,pid;

	/* We use this call to keep track of the references to the module.
	 * With this function we increment the global counter of references for
	 * our module. */
	try_module_get (THIS_MODULE);


	syscall = sys_call_table_original[OPEN];
	thinfo_stats = (struct my_thread *) current_thread_info ();
	pid = current_thread_info ()->task->pid;
	pidstats = &(thinfo_stats->stats[OPEN]);

	/* This is used to control that statistics saved in the structure are still valid. */
	if (pid != thinfo_stats->pid)
		reset_stats (pid, thinfo_stats);

	/* Here we increment all the statistics of this system call, that is:
	 * -) data into the specific thread info for the specific syscall
	 * -) data regarding the global syscall information
	 * -) data into the specific thread info for the global trend of syscalls */
	pidstats->n_calls++;
	syscall_info_table[OPEN].n_calls++;
	thinfo_stats->stats[N_CALLS].n_calls++;

	/* Here we take the time BEFORE the call */
	start = proso_get_cycles ();
	result = syscall (filename, flags, mode);
	/* Here we take the time once again, once the syscall returned */
	end= proso_get_cycles ();

	/* Now the result of the syscall is analyzed
	 * and increment the correct data */
	if (result >= 0){
		pidstats->n_calls_ok++;
		syscall_info_table[OPEN].n_calls_ok++;
		thinfo_stats->stats[N_CALLS].n_calls_ok++;
	}
	else{
		pidstats->n_calls_err++;
		syscall_info_table[OPEN].n_calls_err++;
		thinfo_stats->stats[N_CALLS].n_calls_err++;
	}

	/* We update execution time data */
	pidstats->total += (end - start);
	syscall_info_table[OPEN].total += (end - start);
	thinfo_stats->stats[N_CALLS].total +=(end - start);

	/* With this function we decrement the global counter of references for
	 * our module. */
	module_put (THIS_MODULE);

	return result;
}

long sys_close_local (unsigned int fd){
	long (*syscall) (unsigned int fd);
	unsigned long long start, end;
	struct my_thread *thinfo_stats;
	struct t_info *pidstats;
	int result,pid;

	/* We use this call to keep track of the references to the module.
	 * With this function we increment the global counter of references for
	 * our module. */
	try_module_get (THIS_MODULE);

	syscall = sys_call_table_original[CLOSE];
	thinfo_stats = (struct my_thread *) current_thread_info ();
	pid = current_thread_info ()->task->pid;
	pidstats = &(thinfo_stats->stats[CLOSE]);

	/* This is used to control that statistics saved in the structure are still valid. */
	if (pid != thinfo_stats->pid)
		reset_stats (pid, thinfo_stats);

	/* Here we increment all the statistics of this system call, that is:
	 * -) data into the specific thread info for the specific syscall
	 * -) data regarding the global syscall information
	 * -) data into the specific threead info for the global trend of syscalls */
	pidstats->n_calls++;
	syscall_info_table[CLOSE].n_calls++;
	thinfo_stats->stats[N_CALLS].n_calls++;

	/* Here we take the time BEFORE the call */
	start = proso_get_cycles ();
	result = syscall (fd);
	/* Here we take the time once again, once the syscall returned */
	end= proso_get_cycles ();

	/* Now the result of the syscall is analyzed
	 * and increment the correct data */
	if (result >= 0){
		pidstats->n_calls_ok++;
		syscall_info_table[CLOSE].n_calls_ok++;
		thinfo_stats->stats[N_CALLS].n_calls_ok++;
	}
	else{
		pidstats->n_calls_err++;
		syscall_info_table[CLOSE].n_calls_err++;
		thinfo_stats->stats[N_CALLS].n_calls_err++;
	}

	/* We update execution time data */
	pidstats->total += (end - start);
	syscall_info_table[CLOSE].total += (end - start);
	thinfo_stats->stats[N_CALLS].total +=(end - start);

	/* With this function we decrement the global counter of references for
	 * our module. */
	module_put (THIS_MODULE);

	return result;
}

ssize_t sys_write_local (unsigned int fd, const char /*__user*/ * buf, size_t count){
	ssize_t (*syscall) (unsigned int fd, const char /*__user*/ * buf, size_t count);
	unsigned long long start, end;
	struct my_thread *thinfo_stats;
	struct t_info *pidstats;
	int result,pid;

	/* We use this call to keep track of the references to the module.
	 * With this function we increment the global counter of references for
	 * our module. */
	try_module_get (THIS_MODULE);

	syscall = sys_call_table_original[MYWRITE];
	thinfo_stats = (struct my_thread *) current_thread_info ();
	pid = current_thread_info ()->task->pid;
	pidstats = &(thinfo_stats->stats[MYWRITE]);

	/* This is used to control that statistics saved in the structure are still valid. */
	if (pid != thinfo_stats->pid)
		reset_stats (pid, thinfo_stats);

	/* Here we increment all the statistics of this system call, that is:
	 * -) data into the specific thread info for the specific syscall
	 * -) data regarding the global syscall information
	 * -) data into the specific threead info for the global trend of syscalls */
	pidstats->n_calls++;
	syscall_info_table[MYWRITE].n_calls++;
	thinfo_stats->stats[N_CALLS].n_calls++;

	/* Here we take the time BEFORE the call */
	start = proso_get_cycles ();
	result = syscall (fd,buf,count);
	/* Here we take the time once again, once the syscall returned */
	end= proso_get_cycles ();

	/* Now the result of the syscall is analyzed
	 * and increment the correct data */
	if (result >= 0){
		pidstats->n_calls_ok++;
		syscall_info_table[MYWRITE].n_calls_ok++;
		thinfo_stats->stats[N_CALLS].n_calls_ok++;
	}
	else{
		pidstats->n_calls_err++;
		syscall_info_table[MYWRITE].n_calls_err++;
		thinfo_stats->stats[N_CALLS].n_calls_err++;
	}

	/* We update execution time data */
	pidstats->total += (end - start);
	syscall_info_table[MYWRITE].total += (end - start);
	thinfo_stats->stats[N_CALLS].total +=(end - start);

	/* With this function we decrement the global counter of references for
	 * our module. */
	module_put (THIS_MODULE);

	return result;
}

int sys_clone_local (struct pt_regs regs){
	int (*syscall) (struct pt_regs regs);
	unsigned long long start, end;
	struct my_thread *thinfo_stats;
	struct t_info *pidstats;
	int result,pid;

	/* We use this call to keep track of the references to the module.
	 * With this function we increment the global counter of references for
	 * our module. */
	try_module_get (THIS_MODULE);

	syscall = sys_call_table_original[CLONE];
	thinfo_stats = (struct my_thread *) current_thread_info ();
	pid = current_thread_info ()->task->pid;
	pidstats = &(thinfo_stats->stats[CLONE]);

	/* This is used to control that statistics saved in the structure are still valid. */
	if (pid != thinfo_stats->pid)
		reset_stats (pid, thinfo_stats);

	/* Here we increment all the statistics of this system call, that is:
	 * -) data into the specific thread info for the specific syscall
	 * -) data regarding the global syscall information
	 * -) data into the specific threead info for the global trend of syscalls */
	pidstats->n_calls++;
	syscall_info_table[CLONE].n_calls++;
	thinfo_stats->stats[N_CALLS].n_calls++;

	/* Here we take the time BEFORE the call */
	start = proso_get_cycles ();
	result = syscall (regs);
	/* Here we take the time once again, once the syscall returned */
	end= proso_get_cycles ();

	//printk(KERN_DEBUG "Here I trapped Clone output: %i\n",result);

	/* Now the result of the syscall is analyzed
	 * and increment the correct data */
	if (result >= 0){
		pidstats->n_calls_ok++;
		syscall_info_table[CLONE].n_calls_ok++;
		thinfo_stats->stats[N_CALLS].n_calls_ok++;
	}
	else{
		pidstats->n_calls_err++;
		syscall_info_table[CLONE].n_calls_err++;
		thinfo_stats->stats[N_CALLS].n_calls_err++;
	}

	/* We update execution time data */
	pidstats->total += (end - start);
	syscall_info_table[CLONE].total += (end - start);
	thinfo_stats->stats[N_CALLS].total +=(end - start);

	/* With this function we decrement the global counter of references for
	 * our module. */
	module_put (THIS_MODULE);

	return result;
}

off_t sys_lseek_local (unsigned int fd, off_t offset, unsigned int origin){
	off_t (*syscall) (unsigned int fd, off_t offset, unsigned int origin);
	unsigned long long start, end;
	struct my_thread *thinfo_stats;
	struct t_info *pidstats;
	int result,pid;

	/* We use this call to keep track of the references to the module.
	 * With this function we increment the global counter of references for
	 * our module. */
	try_module_get (THIS_MODULE);

	syscall = sys_call_table_original[LSEEK];
	thinfo_stats = (struct my_thread *) current_thread_info ();
	pid = current_thread_info ()->task->pid;
	pidstats = &(thinfo_stats->stats[LSEEK]);

	/* This is used to control that statistics saved in the structure are still valid. */
	if (pid != thinfo_stats->pid)
		reset_stats (pid, thinfo_stats);

	/* Here we increment all the statistics of this system call, that is:
	 * -) data into the specific thread info for the specific syscall
	 * -) data regarding the global syscall information
	 * -) data into the specific threead info for the global trend of syscalls */
	pidstats->n_calls++;
	syscall_info_table[LSEEK].n_calls++;
	thinfo_stats->stats[N_CALLS].n_calls++;

	/* Here we take the time BEFORE the call */
	start = proso_get_cycles ();
	result = syscall (fd, offset, origin);
	/* Here we take the time once again, once the syscall returned */
	end= proso_get_cycles ();

	/* Now the result of the syscall is analyzed
	 * and increment the correct data */
	if (result >= 0){
		pidstats->n_calls_ok++;
		syscall_info_table[LSEEK].n_calls_ok++;
		thinfo_stats->stats[N_CALLS].n_calls_ok++;
	}
	else{
		pidstats->n_calls_err++;
		syscall_info_table[LSEEK].n_calls_err++;
		thinfo_stats->stats[N_CALLS].n_calls_err++;
	}

	/* We update execution time data */
	pidstats->total += (end - start);
	syscall_info_table[LSEEK].total += (end - start);
	thinfo_stats->stats[N_CALLS].total +=(end - start);

	/* With this function we decrement the global counter of references for
	 * our module. */
	module_put (THIS_MODULE);

	return result;
}

void set_mon_on_syscall (int call_index){
	/* We set the monitor to intercept the call by redirecting
	 * the syscall to our local trap. */
	sys_call_table[local_to_sys_index[call_index]] = sys_call_table_local[call_index];
}

void unset_mon_on_syscall (int call_index){
	/* We unset the monitor from intercept the call by
	 * restoring into the sys_call_table the original
	 * address we saved at the initialization phase.  */
	sys_call_table[local_to_sys_index[call_index]] = sys_call_table_original[call_index];
}

//TODO Controllare storia processi senza statistiche! vedi commento sotto.
/* ma se il processo non ha statistiche (diciamo ke è un processo che non ha ancora
 * chiamato le syscall e quindi la sua parte in alto dello stack NON sono statistiche)...
 * è un problema azzerare tutto? non rischiamo di cancellare dati che fanno parte dello
 * stack del processo?? Come distinguiamo processi ke hanno statistiche da processi
 * ke nn le hanno?
 */
void reset_stats (int pid, struct my_thread *t_info_stats){
  int i;
  struct t_info *pidstats;

  t_info_stats->pid = pid;

  for (i = 0; i <= N_CALLS; i++){
	  pidstats = &(t_info_stats->stats[i]);

	  /* Reset all the statistics */
	  pidstats->n_calls = 0;
	  pidstats->n_calls_ok = 0;
	  pidstats->n_calls_err = 0;
	  pidstats->total = 0;
	}
}

void print_total_sys_stats(void){
	struct t_info pidstats;

	printk ("    -- SysCalls Dumb Monitor --\n");
	printk (" Global statistics for System Calls\n");
	printk ("    -------------\n\n");
	pidstats = syscall_info_table[OPEN];
	printk ("# OPEN Calls      : %i\n", pidstats.n_calls);
	printk ("# OK returned     : %i\n", pidstats.n_calls_ok);
	printk ("# ERR returned    : %i\n", pidstats.n_calls_err);
	printk ("Total exec. time  : %lld\n\n", pidstats.total);
	pidstats = syscall_info_table[CLOSE];
	printk ("# CLOSE Calls     : %i\n", pidstats.n_calls);
	printk ("# OK returned     : %i\n", pidstats.n_calls_ok);
	printk ("# ERR returned    : %i\n", pidstats.n_calls_err);
	printk ("Total exec. time  : %lld\n\n", pidstats.total);
	pidstats = syscall_info_table[MYWRITE];
	printk ("# WRITE Calls     : %i\n", pidstats.n_calls);
	printk ("# OK returned     : %i\n", pidstats.n_calls_ok);
	printk ("# ERR returned    : %i\n", pidstats.n_calls_err);
	printk ("Total exec. time  : %lld\n\n", pidstats.total);
	pidstats = syscall_info_table[CLONE];
	printk ("# CLONE Calls     : %i\n", pidstats.n_calls);
	printk ("# OK returned     : %i\n", pidstats.n_calls_ok);
	printk ("# ERR returned    : %i\n", pidstats.n_calls_err);
	printk ("Total exec. time  : %lld\n\n", pidstats.total);
	pidstats = syscall_info_table[LSEEK];
	printk ("# LSEEK Calls     : %i\n", pidstats.n_calls);
	printk ("# OK returned     : %i\n", pidstats.n_calls_ok);
	printk ("# ERR returned    : %i\n", pidstats.n_calls_err);
	printk ("Total exec. time  : %lld\n\n", pidstats.total);
	printk ("    -------------\n");
}

void print_stats (int pid){
	struct task_struct *task_pid;
	struct t_info *pidstats;

	//Search ofr the process from its PID.
	task_pid = find_task_by_pid ((pid_t) pid);

	if (task_pid >= 0){
		printk ("    -- SysCalls Dumb Monitor --\n");
		printk ("PID               : %i\n", pid);
		printk ("    -------------\n");
		pidstats =&(((struct my_thread *) task_pid->thread_info)->stats[N_CALLS]);
		printk ("# TOTAL Calls     : %i\n", pidstats->n_calls);
		printk ("# OK  returned    : %i\n", pidstats->n_calls_ok);
		printk ("# ERR returned    : %i\n", pidstats->n_calls_err);
		printk ("Total exec. time  : %lld\n", pidstats->total);
		printk ("    -------------\n\n");
		pidstats = &(((struct my_thread *) task_pid->thread_info)->stats[OPEN]);
		printk ("# OPEN Calls      : %i\n", pidstats->n_calls);
		printk ("# OK returned     : %i\n", pidstats->n_calls_ok);
		printk ("# ERR returned    : %i\n", pidstats->n_calls_err);
		printk ("Total exec. time  : %lld\n\n", pidstats->total);
		pidstats = &(((struct my_thread *) task_pid->thread_info)->stats[CLOSE]);
		printk ("# CLOSE Calls     : %i\n", pidstats->n_calls);
		printk ("# OK returned     : %i\n", pidstats->n_calls_ok);
		printk ("# ERR returned    : %i\n", pidstats->n_calls_err);
		printk ("Total exec. time  : %lld\n\n", pidstats->total);
		pidstats = &(((struct my_thread *) task_pid->thread_info)->stats[MYWRITE]);
		printk ("# WRITE Calls     : %i\n", pidstats->n_calls);
		printk ("# OK returned     : %i\n", pidstats->n_calls_ok);
		printk ("# ERR returned    : %i\n", pidstats->n_calls_err);
		printk ("Total exec. time  : %lld\n\n", pidstats->total);
		pidstats = &(((struct my_thread *) task_pid->thread_info)->stats[CLONE]);
		printk ("# CLONE Calls     : %i\n", pidstats->n_calls);
		printk ("# OK returned     : %i\n", pidstats->n_calls_ok);
		printk ("# ERR returned    : %i\n", pidstats->n_calls_err);
		printk ("Total exec. time  : %lld\n\n", pidstats->total);
		pidstats = &(((struct my_thread *) task_pid->thread_info)->stats[LSEEK]);
		printk ("# LSEEK Calls     : %i\n", pidstats->n_calls);
		printk ("# OK returned     : %i\n", pidstats->n_calls_ok);
		printk ("# ERR returned    : %i\n", pidstats->n_calls_err);
		printk ("Total exec. time  : %lld\n\n", pidstats->total);
		printk ("    -------------\n");
	}
	else
		printk ("Syscalls Monitor: No such PID (%i)\n",pid);
}

int get_stats (int pid, int call, struct t_info *stats){
	struct task_struct *t;
	struct my_thread *task;
	struct t_info *task_stats;

	if (call < 0 || call > N_CALLS)
		return -EINVAL;

	if (pid < 0)
		return -EINVAL;

	t = find_task_by_pid ((pid_t) pid);

	/*printk ("t->pid: %i", t->pid); //Debug mode */

	if (t == NULL)
		return -ESRCH;

	task = (struct my_thread *) t->thread_info;

	task_stats = ((struct t_info *) &(task->stats[call]));

	stats->n_calls = task_stats->n_calls;
	stats->n_calls_ok = task_stats->n_calls_ok;
	stats->n_calls_err = task_stats->n_calls_err;
	stats->total = task_stats->total;

	/*/ Debug mode
	printk (KERN_EMERG "\n1=%i\n2=%i\n3=%i\n4=%lld\n\n", stats->n_calls,
													   stats->n_calls_ok,
													   stats->n_calls_err,
													   stats->total); */
	return 0;
}
