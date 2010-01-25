/*
 * sched.h - Structures and macros for process management
 */
#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <mm_address.h>
#include <filesystem.h>

#define NR_TASKS      10
#define KERNEL_STACK_SIZE	1024
#define SEM_MAX_VALUE 26
#define MAX_FDS 10

struct task_struct {
  int pid;
  int quantum;
  int cpu_tics;
  int remaining;
  int transitions;
  /* Data pages of the process */
  int ph_pages[NUM_PAG_DATA];
  /* Semaphores owned by the process */
  int owned_sems[SEM_MAX_VALUE];
  /* Connection with the list of processes */
  struct list_head run_list;

  /* Keyboard management */
  int pending_chars;
  int size;
  char *buffer;

  /* File descriptors of the process */
  struct virtual* file_descriptors[MAX_FDS];
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* Kernel task of a process */
};

struct protected_task_struct {
  unsigned long task_protect[KERNEL_STACK_SIZE];  /* This field protects the different task_structs, so any acces to this field will generate a PAGE FAULT exeption */
  union task_union t;
};
extern struct protected_task_struct task[NR_TASKS];


struct semaphore{
	unsigned int count;
	int init;
	struct list_head wait_list;
};
struct semaphore sem[SEM_MAX_VALUE];

int pid;  /* This will be the PID counter used to assign a unique number to every process */
struct list_head runqueue; /* List of run+ready processes */
int it_is_int; /* this is used to check if a task switch is called by an interrupt or not */

void init_sched(void);
void init_task0(void);

struct task_struct *list_head_to_task_struct (struct list_head *l);
struct task_struct *current();
int srch_free_task(void);
void task_switch(union task_union* t);
void schedule(int it_is_int);


#endif  /* __SCHED_H__ */
