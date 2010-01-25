/*
 * sched.c - initializes struct for task 0
 */

#include <sched.h>
#include <mm.h>
#include <interrupt.h>
#include <list.h>
#include <keyboard.h>

struct protected_task_struct task[NR_TASKS]
  __attribute__((__section__(".data.task")));

/* This function initializes datas and structures needed by the scheduler */
void init_sched(void){
	int i;
	
	INIT_LIST_HEAD(&runqueue);
	INIT_LIST_HEAD (&keyboard_queue);
	pid=0;
	it_is_int=0;
	
	for(i=0;i<NR_TASKS;i++) task[i].t.task.pid=-1;
}


void init_task0(void)
{
  int i, first_ph;
  
/* Initializes paging for the process 0 adress space */
  first_ph=initialize_P0_frames();
  set_user_pages();
  set_cr3();
  
  /* Setting parameters of task0 */
  
  task[0].t.task.pid=pid++;
  task[0].t.task.quantum=60;
  task[0].t.task.cpu_tics=0;
  task[0].t.task.remaining=task[0].t.task.quantum;
  task[0].t.task.transitions=1;
  
  left4current=task[0].t.task.quantum; // It is going to be used by the scheduler

  //This sets all the pages of data to task0 --> è così? CONTROLLARE
  for(i=0;i<NUM_PAG_DATA;i++)
	  //task[0].t.task.ph_pages[i]=first_ph+NUM_PAG_CODE+i;
	  task[0].t.task.ph_pages[i]=alloc_frame();
  
  for(i=0;i<SEM_MAX_VALUE;i++) /* at the beginning, no semaphore is owned by the process */
	  task[0].t.task.owned_sems[i]=-1;

  //Initialization of file descriptors for the process 0
  for (i = 3; i < MAX_FDS; i++){
	  task[0].t.task.file_descriptors[i] = NULL;
  }
  /* Initialization of the 3 standard FDs.
   * stdin 	= 0
   * stdout = 1
   * stderr = 2
   *
   * We initialize the first 2 entry of FDT to KEYBOARD and
   * DISPLAY respectively.
   *
   * We redirect stderr to the console (dir[1]).
   **/
  fdt[0].refs = 1;
  fdt[0].access_mode = O_RDONLY;
  fdt[0].offset = 0;
  fdt[0].logic_dev = &dir[0];
  fdt[0].logic_dev->n_refs = 1;

  fdt[1].refs = 1;
  fdt[1].access_mode = O_WRONLY;
  fdt[1].offset = 0;
  fdt[1].logic_dev = &dir[1];
  fdt[1].logic_dev->n_refs = 1;

  task[0].t.task.file_descriptors[0] = &fdt[0];
  task[0].t.task.file_descriptors[1] = &fdt[1];
  task[0].t.task.file_descriptors[2] = &fdt[1];
  //End of file descriptors management

  list_add(&(task[0].t.task.run_list),&runqueue);
  
}

struct task_struct *list_head_to_task_struct (struct list_head *l){
	return list_entry (l,struct task_struct,run_list);
}

struct task_struct *current(){
	struct task_struct *c;
	
	__asm__ __volatile__ ("movl $0xfffff000, %%ecx\n"
                              "andl %%esp, %%ecx\n"
                              "movl %%ecx, %0\n": "=g" (c)::"cx");
        /* For being sure, I'm saying that %ecx is being modified...is it mandatory? isn't it already saved by system (C conventions)??? */
	return c;
}

/* This function searches for a free task in the array of tasks */
int srch_free_task(void){
	int i;
	
	for(i=0;i < NR_TASKS; i++)
		if(task[i].t.task.pid<0) return i;
	
	return -1;
		
}

/* This function performs the change of context between the current process
 * and the one pointed by t. */
void task_switch(union task_union* t){
	int i;

	/* Increments the number of transitions between READY and RUN of the task */
	t->task.transitions++;

	/* Updates the execution time left to the new process */
	left4current = t->task.quantum;

	t->task.remaining=t->task.quantum;

	/* Updates the TSS with the address of t stack */
	tss.esp0 = (DWord) & (t->stack[KERNEL_STACK_SIZE]);

	/* Here the address space is changed by means of the page table.
	 * We have to map every physical page of t to a logical page.
	 * Then we flush TLB */
	for(i=0;i<NUM_PAG_DATA && t->task.ph_pages[i]!=-1; i++){
		set_ss_pag(PAG_LOG_INIT_DATA_P0+i, (unsigned) t->task.ph_pages[i]);
	}
	set_cr3();

	/* Change the stack pointer to the system stack of t */
	__asm__ __volatile__ ("movl %0, %%esp\n"::"g"((DWord)&(t->stack[KERNEL_STACK_SIZE-16])));

	if(it_is_int==1){
		/* task_switch is called by an interrupt.
		 * we have to send an EOI to be able to receive other interrupts once
		 * the context of the new process is restored.*/
		__asm__ __volatile__ ("movb $0x20, %al\n"
							  "outb %al, $0x20\n");

		it_is_int=0;
	}

	/* Restore the context of the new process.
	 * These lines consist in the RESTORE_ALL macro and the execution
	 * is actually restored with the IRET instruction. */
	__asm__ __volatile__("popl %ebx;\n"
						"popl %ecx;\n"
						"popl %edx;\n"
						"popl %esi;\n"
						"popl %edi;\n"
						"popl %ebp;\n"
						"popl %eax;\n"
						"popl %ds;\n"
						"popl %es;\n"
						"popl %fs;\n"
						"popl %gs;\n"
						"IRET\n");
}

void schedule(int from_int){
	struct task_struct *old_task;
	union task_union *new_task;

	old_task=current();

	/* Checks if the time given to the current process is over */
	if(left4current<=0){
		/* Checks if runqueue only contains the init process */
		if(list_first(&runqueue) == list_last(&runqueue)){
			left4current=old_task->quantum;
			old_task->remaining=left4current;
		}
		else{ /* If there is more than one process... */
			/* If the scheduler is called by sys_exit,
			the process will be already unset, waiting for a context switch */
			if(old_task->pid!=-1){
				/* deletes the current process from runqueue */
				list_del (&(old_task->run_list));
				/* reinserts it at the end of runqueue */
				list_add_tail (&(old_task->run_list), &runqueue);
			}
			new_task = (union task_union *) (list_head_to_task_struct (runqueue.next));
			it_is_int = from_int;
			/* changes context to the next process */
			task_switch (new_task);
		}
	}
}
