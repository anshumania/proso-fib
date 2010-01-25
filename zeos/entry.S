/*
 * entry.S - Entry point to system mode from user mode
 */

#include <asm.h>
#include <segment.h>
#include <errno.h>


/**************************************************/
/**** Save & Restore ******************************/
/**                                              **/
/** When we change to privilege level 0 (kernel) **/
/** (through an interrupt, a system call, an     **/
/** exception ...) we must save the state of the **/
/** currently running task (save).               **/
/**                                              **/
/** Stack layout in 'systemCall':                **/
/**                                              **/
/**   0(%esp) - %ebx    \                        **/
/**   4(%esp) - %ecx     |                       **/
/**   8(%esp) - %edx     |                       **/
/**   C(%esp) - %esi     | Register saved        **/
/**  10(%esp) - %edi     |  by 'save'            **/
/**  14(%esp) - %ebp     |                       **/
/**  18(%esp) - %eax     |                       **/
/**  1C(%esp) - %ds      |                       **/
/**  20(%esp) - %es      |                       **/
/**  24(%esp) - %fs      |                       **/
/**  28(%esp) - %gs     /                        **/
/**  2C(%esp) - %eip    \                        **/
/**  30(%esp) - %cs      |                       **/
/**  34(%esp) - %eflags  |  Return context saved **/
/**  38(%esp) - %oldesp  |   by the processor.   **/
/**  3C(%esp) - %oldss  /                        **/
/**                                              **/
/**************************************************/

#define SAVE_ALL \
      pushl %gs; \
      pushl %fs; \
      pushl %es; \
      pushl %ds; \
      pushl %eax; \
      pushl %ebp; \
      pushl %edi; \
      pushl %esi; \
      pushl %edx; \
      pushl %ecx; \
      pushl %ebx; \
      movl $__KERNEL_DS, %edx;    \
      movl %edx, %ds;           \
      movl %edx, %es

#define RESTORE_ALL \
	popl %ebx; \
	popl %ecx; \
	popl %edx; \
	popl %esi; \
	popl %edi; \
	popl %ebp; \
	popl %eax; \
	popl %ds; \
	popl %es; \
	popl %fs; \
	popl %gs;
	
#define EOI \
	movb $0x20, %al; \
	outb %al, $0x20;
	
#define MINSYSCALL 0
#define MAXSYSCALL 41
	
/* Exception handlers*/
	
ENTRY(divide_error_handler)
	SAVE_ALL
	call divide_error_routine
	RESTORE_ALL
	IRET

ENTRY(debug_handler)
	SAVE_ALL
	call debug_routine
	RESTORE_ALL
	IRET

ENTRY(nm1_handler)
	SAVE_ALL
	call nm1_routine
	RESTORE_ALL
	IRET

ENTRY(breakpoint_handler)
	SAVE_ALL
	call breakpoint_routine
	RESTORE_ALL
	IRET

ENTRY(overflow_handler)
	SAVE_ALL
	call overflow_routine
	RESTORE_ALL
	IRET

ENTRY(bounds_check_handler)
	SAVE_ALL
	call bounds_check_routine
	RESTORE_ALL
	IRET

ENTRY(invalid_opcode_handler)
	SAVE_ALL
	call invalid_opcode_routine
	RESTORE_ALL
	IRET

ENTRY(device_not_available_handler)
	SAVE_ALL
	call device_not_available_routine
	RESTORE_ALL
	IRET

ENTRY(double_fault_handler)
	SAVE_ALL
	call double_fault_routine
	RESTORE_ALL
	addl $4, %esp
	IRET

ENTRY(coprocessor_segment_overrun_handler)
	SAVE_ALL
	call coprocessor_segment_overrun_routine
	RESTORE_ALL
	IRET

ENTRY(invalid_tss_handler)
	SAVE_ALL
	call invalid_tss_routine
	RESTORE_ALL
	addl $4, %esp
	IRET

ENTRY(segment_not_present_handler)
	SAVE_ALL
	call segment_not_present_routine
	RESTORE_ALL
	addl $4, %esp
	IRET

ENTRY(stack_exception_handler)
	SAVE_ALL
	call stack_exception_routine
	RESTORE_ALL
	addl $4, %esp
	IRET
	
ENTRY(general_protection_handler)
	SAVE_ALL
	call general_protection_routine
	RESTORE_ALL
	addl $4, %esp
	IRET	

ENTRY(page_fault_handler)
	SAVE_ALL
	call page_fault_routine
	RESTORE_ALL
	addl $4, %esp
	IRET	

ENTRY(intel_reserved_handler)
	SAVE_ALL
	/*call intel_protection_routine*/
	RESTORE_ALL
	IRET

ENTRY(floating_point_error_handler)
	SAVE_ALL
	call floating_point_error_routine
	RESTORE_ALL
	IRET

ENTRY(alignment_check_handler)
	SAVE_ALL
	call alignment_check_routine
	RESTORE_ALL
	addl $4, %esp
	IRET

/* Interrupt handlers */

ENTRY(clock_handler)
	SAVE_ALL
	call clock_routine
	EOI
	RESTORE_ALL
	IRET

ENTRY(keyboard_handler)
	SAVE_ALL
	call keyboard_routine
	EOI
	RESTORE_ALL
	IRET
	
ENTRY(system_call_handler)
	SAVE_ALL
	cmpl $MINSYSCALL,%eax
	jl error_no_syscall
	cmpl $MAXSYSCALL,%eax
	jg error_no_syscall
	call *sys_call_table(,%eax,0x4)
	movl %eax, 24(%esp)
	RESTORE_ALL
	IRET

error_no_syscall:
	movl $-ENOSYS, 24(%esp)
	RESTORE_ALL
	IRET

ENTRY(sys_call_table)
	.long sys_ni_syscall	/* syscall not implemented */
	.long sys_exit			/* call to exit routine			#1 */
	.long sys_fork			/* call to fork routine 		#2 */
	.long sys_read			/* call to read independent routine #3 */
	.long sys_write			/* call to write independent routine #4 */
	.long sys_open			/* call to open routine #5 */
	.long sys_close			/* call to close routine #6 */
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_unlink		/* call to unlink routine #10 */
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_getpid		/* call to getpid routine 		#20 */
	.long sys_sem_init		/* call to sem_init routine 	#21 */
	.long sys_sem_wait		/* call to sem_wait routine 	#22 */
	.long sys_sem_signal	/* call to sem_signal routine 	#23 */
	.long sys_sem_destroy	/* call to sem_destroy routine 	#24 */
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_nice			/* call to nice routine 		#34 */
	.long sys_get_stats		/* call to get_stats_routine	#35 */
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_ni_syscall
	.long sys_dup			/* call to dup routine			#41 */
