/*
 * interrupt.c -
 */
 
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>
#include <entry.h>
#include <utils.h>
#include <sys.h>
#include <sched.h>
#include <devices.h>
#include <keyboard.h>


Gate idt[IDT_ENTRIES];
Register idtR;
int tics;

char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','¡','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','ñ',
  '\0','º','\0','ç','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void iniTics(){
	tics=-1;
}

void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  /*MODIFICATION: Here the IDT entries are initialized.*/
  
  //Exceptions
	setInterruptHandler(0,divide_error_handler,0);
	setInterruptHandler(1,debug_handler,0);
	setInterruptHandler(2,nm1_handler,0);
	setInterruptHandler(3,breakpoint_handler,0);
	setInterruptHandler(4,overflow_handler,0);
	setInterruptHandler(5,bounds_check_handler,0);
	setInterruptHandler(6,invalid_opcode_handler,0);
	setInterruptHandler(7,device_not_available_handler,0);
	setInterruptHandler(8,double_fault_handler,0);
	setInterruptHandler(9,coprocessor_segment_overrun_handler,0);
	setInterruptHandler(10,invalid_tss_handler,0);
	setInterruptHandler(11,segment_not_present_handler,0);
	setInterruptHandler(12,stack_exception_handler,0);
	setInterruptHandler(13,general_protection_handler,0);
	setInterruptHandler(14,page_fault_handler,0);
	setInterruptHandler(15,intel_reserved_handler,0);
	setInterruptHandler(16,floating_point_error_handler,0);
	setInterruptHandler(17,alignment_check_handler,0);
	
  //Interrupts
    setInterruptHandler(32,clock_handler,0);
    setInterruptHandler(33,keyboard_handler,0);
    
  //System call
    setTrapHandler(128,system_call_handler,3);
    
    
	/*	*/
  set_idt_reg(&idtR);
}

void divide_error_routine(){
	printk("Exeption n.0: Divide error\n");
	while(1);
}

void debug_routine(){
	printk("Exeption n.1: Debug\n");
	while(1);
}

void nm1_routine(){
	printk("Exeption n.2: NM1\n");
	while(1);
}

void breakpoint_routine(){
	printk("Exeption n.3: Breakpoint\n");
	while(1);
}

void overflow_routine(){
	printk("Exeption n.4: Overflow\n");
	while(1);
}

void bounds_check_routine(){
	printk("Exeption n.5: Bounds Check\n");
	while(1);
}

void invalid_opcode_routine(){
	printk("Exeption n.6: Invalid Opcode\n");
	while(1);
}

void device_not_available_routine(){
	printk("Exeption n.7: Device not Available\n");
	while(1);
}

void double_fault_routine(){
	printk("Exeption n.8: Double Fault\n");
	while(1);
}

void coprocessor_segment_overrun_routine(){
	printk("Exeption n.9: Coprocessor Segment Overrun\n");
	while(1);
}

void invalid_tss_routine(){
	printk("Exeption n.10: Invalid TSS\n");
	while(1);
}

void segment_not_present_routine(){
	printk("Exeption n.11: Segment not Present\n");
	while(1);
}

void stack_exception_routine(){
	printk("Exeption n.12: Stack Exception\n");
	while(1);
}

void general_protection_routine(){
	printk("Exeption n.13: General protection\n");
	while(1);
}

void page_fault_routine(){
	int pid_fault = current ()->pid;

	printk ("PAGE FAULT:\nThe process with PID=");
	/* A char is enough because the maximum number of processes is 10. */
	printc ('0' + pid_fault);
	printk (" generated this exception and has been terminated\n");

	if (pid_fault != 0)
		sys_exit ();
	else
		while (1);
}


/*
void intel_protection_routine(){
	printk("Exeption n.15: Intel Reserved\n");
	while(1);
}
*/

void floating_point_error_routine(){
	printk("Exeption n.16: Floating Point Error\n");
	while(1);
}

void alignment_check_routine(){
	printk("Exeption n.17: Alignment check\n");
	while(1);
}

/* Interrupts routines */

void clock_routine(){
	int minutes;
	int seconds;
	char time[]={'0','0',':','0','0','\0'};
	struct task_struct *old_task;
	
	old_task= current();

	tics++;
	left4current--;
	old_task->cpu_tics++;
	old_task->remaining--;
	
	schedule(1);

	if((tics%18)==0){
		seconds = tics/18; // these are the total seconds of execution
		minutes = seconds / 60; // these are the minutes of execution
		seconds = seconds % 60; /* these are the seconds passed after the last minute.
								   doing this way, we save a division to the system */
		
		/* the counter is reset after one hour */
		if(minutes>59){
			tics=0;
			minutes=0;
		}
		
		itoa(minutes/10, &time[0]);
		itoa(minutes%10, &time[1]);
		//time[2]=':';
		itoa(seconds/10, &time[3]);
		itoa(seconds%10, &time[4]);
		//time[5]='\0';
		printk_xy(72,8,time);
	}
}

/*void keyboard_routine_old(){
	Byte i = inb (0x60);
	char c;

	if(!(i & 0x80)){  XXXXXXXX && 10000000
						This masks all the bits but the first. If it is a 0 then it is a Make and it got to be printed
		clean_line(24);

		c= char_map[i & 0x7f];  XXXXXXXX && 01111111
		if(c!='\0')
			printc_xy(77,24,c);
		else
			printk_xy(71,24,"Control");


	}
}*/

void keyboard_routine(){
	Byte i = inb (0x60);
	char c;
	struct task_struct *blocked;
	c= char_map[i & 0x7f]; /* XXXXXXXX && 01111111 */

	if(!(i & 0x80)){ /* XXXXXXXX && 10000000 
						This masks all the bits but the first. If it is a 0 then it is a Make and it got to be printed */
		circular[p] = c;
		step_circ_buffer ();
		circ_chars++;

		if (!list_empty (&keyboard_queue)){
			/* There are blocked processes waiting for these chars*/
			blocked = list_head_to_task_struct (keyboard_queue.next);
			blocked->pending_chars--;

			if (blocked->pending_chars <= 0){
				/* Buffer is not full, but we have all requested chars already */
				copy_from_circ_buffer (blocked, blocked->size);
				circ_chars = 0;
				//step_circ_buffer ();
				start = p;
				unblock_and_enqueque (blocked);
				}
			else if (circ_chars >= CIRCULAR_SIZE){
				copy_from_circ_buffer (blocked, CIRCULAR_SIZE);
				blocked->size -= circ_chars;
				blocked->buffer += circ_chars;
				circ_chars = 0;
				//step_circ_buffer ();
				start = p;
				}
			}
		else /* If there is no process waiting, we just print the char */
			printc (c);

	}	
}
