/*
 * sys.c - Syscalls implementation
 */

#include <devices.h>
#include <errno.h>
#include <io.h>
#include <utils.h>
#include <sched.h>
#include <mm.h>
#include <list.h>
#include <interrupt.h>
#include <stats.h>

/* Unimplemented sys call */
int sys_ni_syscall(){
	return -ENOSYS;
}

int sys_write(int fd,char *buffer, int size)
{
	int return_write=0;
	struct task_struct *current_task = current ();
	struct virtual *open_file;
	
	/* These controls aim to check input parameters */

	if (fd < 0 || fd >= MAX_FDS)
		return -EBADF;

	if (current_task->file_descriptors[fd] == NULL)
		  return -EBADF;

	if (size > (NUM_PAG_DATA * PAGE_SIZE) - KERNEL_STACK_SIZE)
		  return -EFBIG;

	if(size<0)
		return -EINVAL;
	
	open_file = current_task->file_descriptors[fd];

	if (open_file->access_mode == O_RDONLY)
	  return -EBADF;

	if(access_ok(1,buffer,size)<1)
		return -EFAULT;
	
	if (open_file->logic_dev->operations->sys_write_dev != NULL)
		return return_write =(*(open_file->logic_dev->operations->sys_write_dev)) (fd, buffer, size);
	else
		return -EINVAL;
}

int sys_getpid(void){
	struct task_struct *p = current();

	return p->pid;
}

int sys_fork(void){
	int i,j;
	int new_task_space;
	int frames[NUM_PAG_DATA];
	
	/* This searches for a free task to assign to the new process.
	 * If there is none, an error is returned.*/
	new_task_space=srch_free_task();
	if(new_task_space==-1) return -EAGAIN;
	
	/* Copies the father structure to the new task, which will be the child process. */
	copy_data(current(),&(task[new_task_space].t),4096);
	
	/* This cycle allocates phisical pages for the new process.
	 * It also controls that, if not enough pages are available, all modifications
	 * are cleared and an error is returned. */
	for(i=0;(i < NUM_PAG_DATA) && (current()->ph_pages[i]!=-1);i++){
		frames[i]=alloc_frame();
		
		/* In case the Nth frame cannot be allocated, all previously allocated frames must be freed and an error is returned. */
		if(frames[i]==-1){ 
			for(j=(i-1);j>=0;j--) 
				free_frame(frames[j]);
			
			/* Unset the new process structure */
			task[new_task_space].t.task.pid=-1;	
			return -EAGAIN;
		}		
	}
	
	/* For every data+stack page...*/
	for(j=0;(j<NUM_PAG_DATA)&&(j<=i);j++){
		
		/* Associates child physical page to father logical page to
		 * permit the former to copy all its data+stack.
		 * (this mapping is temporal)*/
		set_ss_pag(PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA+j,frames[j]);
		
		/* Copies data from father to child physical page */
		copy_data((void *) (PAGE_SIZE*(PAG_LOG_INIT_DATA_P0+j)),
				  (void *)(PAGE_SIZE*(PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA+j)),
				  PAGE_SIZE);
		
		/* Assigns the new physical page to child, making him able to
		 * access its data+stack. */
		task[new_task_space].t.task.ph_pages[j]=frames[j];
					  
		/* Deletes the temporal mapping between father's logical pages
		 * and child's physical pages. */
		del_ss_pag(PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA+j);
	}
	/* Flushes the TLB */
	set_cr3();
	
	/* Here we modify the return value of the fork() for the child.
	 * 0 has to be returned to the child when it will execute,
	 * so we modify %eax (which is actually stacked at the 10th position)*/
	task[new_task_space].t.stack[KERNEL_STACK_SIZE-10]=0;
	
	/* Initialization of all the not shared attributes of the child. */
	task[new_task_space].t.task.pid=pid++;
	task[new_task_space].t.task.quantum=current()->quantum;
	task[new_task_space].t.task.cpu_tics=0;
	task[new_task_space].t.task.transitions=0;
	task[new_task_space].t.task.remaining=task[new_task_space].t.task.quantum;
	
	/* at the beginning, no semaphore is owned by the child */
	for(i=0;i<SEM_MAX_VALUE;i++)
		  task[new_task_space].t.task.owned_sems[i]=-1;

	/* We have to increment the reference field in FDT and in the Directory.
	 * New process will share virtual devices with the father. */
	for (i = 0; i < MAX_FDS; i++)
	{
	  if (task[new_task_space].t.task.file_descriptors[i] != NULL)
		{
		  task[new_task_space].t.task.file_descriptors[i]->refs++;
		  task[new_task_space].t.task.file_descriptors[i]->logic_dev->n_refs++;
		}
	}

	/* Adds the child to the runqueue. */
	list_add_tail(&(task[new_task_space].t.task.run_list),&runqueue);
	/* End of child initialization */

	/* Returns child PID to the father. */
	return pid - 1;
}

/* This syscall initializes the semaphore */
int sys_sem_init (int n_sem, unsigned int value)
{

	/* Checks if the input identifier is valid*/
	if (n_sem < 0 || n_sem >= SEM_MAX_VALUE)
		return -EINVAL;

	/* Checks if the semaphore is already initialized (init=1) */
	if (sem[n_sem].init == 1)
		return -EBUSY;

	sem[n_sem].count = value;
	sem[n_sem].init = 1;

	/* This sets the semaphore as owned by the current process */
	current()->owned_sems[n_sem]=1;

	/* Initializes the list entry of the semaphore */
	INIT_LIST_HEAD (&(sem[n_sem].wait_list));

	return 0;
}

/* This system call allows to control the execution of a process through
 * a semaphore n_sem. If the semaphore counter reaches 0 or less, the current
 * process is blocked and put on the semaphore queue.
 * NOTE: the init process can never be blocked! */
int sys_sem_wait (int n_sem)
{
	struct task_struct *old_task;
	union task_union *new_task;

	old_task = current ();

	/* Checks if the current process is the init process (task0).
	 * Being this process peculiar (cannot be blocked), an error is
	 * returned in this case. */
	if (old_task->pid == 0)
		return -EPERM;

	/* Checks if the input identifier is valid */
	if (n_sem < 0 || n_sem >= SEM_MAX_VALUE || sem[n_sem].init != 1)
		return -EINVAL;

	/* Checks the state of the count of the semaphore*/
	if (sem[n_sem].count <= 0)
	{
	  /* Deletes the process from the runqueue list and puts it at the tail
	   * of the semaphore blocked list */
	  list_del (&(old_task->run_list));
	  list_add_tail (&(old_task->run_list), &(sem[n_sem].wait_list));

	  /* Gets the next runnable task and switches context to it */
	  new_task =(union task_union *) (list_head_to_task_struct (runqueue.next));
	  it_is_int = 0;
	  task_switch (new_task);
	}
	else
		sem[n_sem].count--;

	return 0;
}

/* This system call increments the value of the counter of n_sem if and only if
 * there are no processes in the wait_list of the semaphore.
 * On the other hand, if there are processes, the first of them is unblocked */
int sys_sem_signal (int n_sem)
{
	struct list_head *blocked_task;
	union task_union *blocked_task_union;

  /* Checks if the input identifier is valid */
	if (n_sem < 0 || n_sem >= SEM_MAX_VALUE || sem[n_sem].init != 1)
		return -EINVAL;

	/* Checks if the semaphore list is not empty*/
	if (!list_empty (&(sem[n_sem].wait_list)))
	{
		blocked_task = sem[n_sem].wait_list.next;
		list_del (blocked_task);

		blocked_task_union = (union task_union *) (list_head_to_task_struct (blocked_task));

		/* Modifies %eax so that it contains the return value of sys_wait */
		blocked_task_union->stack[KERNEL_STACK_SIZE - 10] = 0;

		list_add_tail (blocked_task, &runqueue);
	}
	else
		sem[n_sem].count++;

	return 0;
}

/* This system call destroys a semaphore n_sem if this is initialized.
 * If */
int sys_sem_destroy (int n_sem)
{
	struct list_head *blocked_task;
	union task_union *blocked_task_union;
	struct task_struct *running=current();

	/* Checks if the input identifier is valid */
	if (n_sem < 0 || n_sem >= SEM_MAX_VALUE)
		return -EINVAL;

	/* Checks if the semaphore is initialized and if the current process owns the semaphore */
	if(sem[n_sem].init != 1)
			return -EINVAL;
	else
		if(running->owned_sems[n_sem] != 1)
			return -EPERM;

	while (!list_empty (&sem[n_sem].wait_list)){
		/* If there are processes in the queue of this semaphore, they will be
		 * unblocked and will be returned by sem_wait with a EINTR error code
		 */
			blocked_task = sem[n_sem].wait_list.next;
			list_del (blocked_task);

			blocked_task_union = (union task_union *) (list_head_to_task_struct (blocked_task));

			/* Modifies %eax so that it contains a -1 error
			 * code as return value of sys_wait */
			blocked_task_union->stack[KERNEL_STACK_SIZE - 10] = -1;

			list_add_tail (blocked_task, &runqueue);
	}

	sem[n_sem].init = 0;
	running->owned_sems[n_sem]=-1;

	return 0;
}

void sys_exit ()
{
	int i;
	/* Selects the current running process, which will be terminated */
	struct task_struct *running = current ();

	/* Points to the task_union structure of the next runnable task in the runqueue */
	//union task_union *next_task = (union task_union *) list_head_to_task_struct (running->run_list.next);

	if (running->pid != 0){ /* Exit CAN NOT terminate Task0 */
		/* Destroys semaphores owned by current process */
		for (i = 0; i < SEM_MAX_VALUE; i++)
			if(running->owned_sems[i]==1){
				sys_sem_destroy(i);
			}

		/* Sets the task as free */
		running->pid = -1;

		/* Deletes the current process from the list of runnable processes */
		list_del (&running->run_list);

		/* Frees all physical pages assigned to the process */
		for (i = 0; i < NUM_PAG_DATA; i++)
			free_frame (running->ph_pages[i]);

		/* Forces the left execution time to 0 (forces the process to be scheduled)*/
		left4current=0;
		/* Calls the scheduler to choose which process must execute */
		schedule(0);
	}
}

int sys_nice (int quantum){
	int old = -EINVAL;            /* Invalid argument */

	struct task_struct *current_task = current ();

	if (quantum > 0){
		old = current_task->quantum;
		current_task->quantum = quantum;
	}
	return old;
}

int sys_get_stats (int spid, struct stats *st){
	int i = 0;
	struct stats spid_stats;

	/* Invalid argument */
	if (spid < 0)
	return -EINVAL;

	i=access_ok(0,st,16);
	if(i<1){
		//printk("EFAULT: Bad Address\n");
		return -EFAULT;}

	/* Searches for the process with PID=spid */
	for (i = 0; i < NR_TASKS && task[i].t.task.pid != spid; i++);

	/* If no process  is found, returns an error */
	/* No such process */
	if (task[i].t.task.pid != spid)
		return -ESRCH;

	spid_stats.total_tics= task[i].t.task.cpu_tics;
	spid_stats.total_trans= task[i].t.task.transitions;
	spid_stats.remaining_tics=task[i].t.task.remaining;


	return copy_to_user (&(spid_stats), st, sizeof(struct stats));

}

/*
 * Note that the flag value for sys_open means:
 *      0x1 - read-only
 *      0x2 - write-only
 *      0x3 - read-write
 *      0x4 - create
 *      0x8 - EXCL
 */
int sys_open (const char *path, int flags){

	int fd, file_entry, fdt_entry;
	struct task_struct *proc = current ();
	struct file *file;

	if(check_address(path,1)==0)
		return -EFAULT;

	if (path== NULL)
		return -EFAULT;

	if (strlen (path) > MAX_NAME_LENGTH)
		return -ENAMETOOLONG;

	/* Checks if it is possible to open more files */
	for (fdt_entry = 0; fdt_entry < MAX_OPEN_FILES && fdt[fdt_entry].refs > 0; fdt_entry++);
	if (fdt_entry == MAX_OPEN_FILES)
		return -ENFILE;

	/* Checks if the process can have a new file descriptor assigned */
	for (fd = 0; fd < MAX_FDS && proc->file_descriptors[fd] != NULL; fd++);
	if (fd == MAX_FDS)
		return -EMFILE;

	if(flags < 0 || flags > (O_RDONLY | O_WRONLY | O_RDWR | O_CREAT | O_EXCL))
		return -EINVAL;

	/* Looks up for the specified file in the Directory */
	for (file_entry = 0; file_entry < MAX_FILES && (strcmp (dir[file_entry].name, path)); file_entry++);
	if (file_entry == MAX_FILES)
		/* If we're not creating a new file, then searched file doesn't exist.
		 * return error.*/
		if (flags < O_CREAT)
			return -ENOENT;
		else{
			/* Creates a new file */
			file = create_file (path);
			if (file < 0)
				return (int) file;
			if(flags>=O_EXCL) flags-=O_EXCL;
			flags -= O_CREAT;
			}
	else
		if((flags >= (O_EXCL | O_CREAT))) /* if O_EXCL and O_CREAT are set return an error */
			return -EEXIST;
		else{ /* otherwise just open the file */
			file = &dir[file_entry];
			if(flags>=O_CREAT) flags-=O_CREAT;
		}

	/* Dependent routine will be called only if it is implemented for the specific device */
	if (file->operations->sys_open_dev != NULL)
		(*(file->operations->sys_open_dev)) (path, flags);

	/* Checks access permissions */
	if (file->access_mode_allow != O_RDWR && flags != file->access_mode_allow)
		return -EACCES;

	/* Creates a new entry in the FDT and connects it to the logical device */
	fdt[fdt_entry].refs = 1;
	fdt[fdt_entry].access_mode = flags;
	fdt[fdt_entry].offset = 0;
	fdt[fdt_entry].logic_dev = file;
	file->n_refs++;

	proc->file_descriptors[fd] = &fdt[fdt_entry];

	return fd;
}

int sys_read (int fd, char *buffer, int size){

	struct task_struct *current_task = current ();
	struct virtual *open_file;
	int return_read;


	if (fd < 0 || fd >= MAX_FDS)
	  return -EBADF;

	if (current_task->file_descriptors[fd] == NULL)
	  return -EBADF;

	if (size < 0)
	  return -EINVAL;

	if (size > (NUM_PAG_DATA * PAGE_SIZE) - KERNEL_STACK_SIZE)
	  return -EFBIG;

	open_file = (current_task->file_descriptors[fd]);

	if (open_file->access_mode == O_WRONLY)
	  return -EBADF;

	if(access_ok(0,buffer,size)<1) return -EFAULT;

	current_task->size = size;    //Keyboard management
	current_task->buffer = buffer;


	if (open_file->logic_dev->operations->sys_read_dev == NULL) return -ENOSYS;

	return_read = (*(open_file->logic_dev->operations->sys_read_dev)) (fd,buffer,size);

	return return_read;
}

int sys_dup (int fd){

	int new_fd;
	struct task_struct *proc = current ();

	if (fd < 0 || fd >= MAX_FDS || proc->file_descriptors[fd] == NULL)
		return -EBADF;

	for (new_fd = 0; new_fd < MAX_FDS && proc->file_descriptors[new_fd] != NULL; new_fd++);
	if (new_fd == MAX_FDS)
		return -EMFILE;

	proc->file_descriptors[new_fd] = proc->file_descriptors[fd];
	proc->file_descriptors[new_fd]->refs++;
	proc->file_descriptors[new_fd]->logic_dev->n_refs++;

	return new_fd;
}

int sys_close (int fd){
  struct task_struct *current_task= current();

  if (fd < 0 || fd >= MAX_FDS || current_task->file_descriptors[fd] == NULL)
	  return -EBADF;

  // Uses the dependent routine if the device implements it
  if (current_task->file_descriptors[fd]->logic_dev->operations->sys_close_dev != NULL)
	  (current_task->file_descriptors[fd]->logic_dev->operations->sys_close_dev) (fd);

  // Closes the generic virtual device
  current_task->file_descriptors[fd]->refs--;
  current_task->file_descriptors[fd]->logic_dev->n_refs--;
  current_task->file_descriptors[fd] = NULL;

  return 0;
}

int sys_unlink (const char *path){
  struct file *file;
  int file_entry;

  if(check_address(path,1)==0)
  		return -EFAULT;

  if (strlen (path) > MAX_NAME_LENGTH)
    return -ENAMETOOLONG;

  for (file_entry = 0;
       file_entry < MAX_FILES && (strcmp (dir[file_entry].name, path));
       file_entry++);
  if (file_entry == MAX_FILES)
    return -ENOENT;

  file = &dir[file_entry];

  if (file->n_refs == 0){
	  if (file->operations->sys_release_dev != NULL)
        (file->operations->sys_release_dev) (file_entry);

      file->operations->sys_open_dev = NULL;
      file->operations->sys_close_dev = NULL;
      file->operations->sys_read_dev = NULL;
      file->operations->sys_write_dev = NULL;
      file->operations->sys_release_dev = NULL;

      file->n_refs = -1;
      file->name[0] = 0;

      return 0;
    }

  return -EBUSY;
}
