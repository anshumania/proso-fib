/*
 * mm.c - Memory Management: Paging & segment memory management
 */

#include <types.h>
#include <mm.h>
#include <mm_address.h>
#include <segment.h>
#include <hardware.h>
#include <sched.h>

Byte phys_mem[TOTAL_PAGES];

/* SEGMENTATION */
/* Memory segements description table */
Descriptor  *gdt = (Descriptor *) GDT_START;
/* Register pointing to the memory segments table */
Register    gdtR;

/* PAGING */
/* Variables containing the page directory and the page table */
  
page_table_entry dir_pages[TOTAL_PAGES]
  __attribute__((__section__(".data.task")));

page_table_entry pagusr_table[TOTAL_PAGES]
  __attribute__((__section__(".data.task")));

/* TSS */
TSS         tss; 



/***********************************************/
/************** PAGING MANAGEMENT **************/
/***********************************************/

/* This function allows to set a whole entry of the Page Table to 0 */
void del_ss_pag(unsigned logical_page){
	pagusr_table[logical_page].entry=0;
}

/* Init page table directory */
  
void init_dir_pages()
{
  dir_pages[ENTRY_DIR_PAGES].entry = 0;
  dir_pages[ENTRY_DIR_PAGES].bits.pbase_addr = (((unsigned int)&pagusr_table) >> 12);
  dir_pages[ENTRY_DIR_PAGES].bits.user = 1;
  dir_pages[ENTRY_DIR_PAGES].bits.rw = 1;
  dir_pages[ENTRY_DIR_PAGES].bits.present = 1;
}

/* Initializes the page table (kernel pages only) */
void init_table_pages()
{
  int i;
  unsigned int page;
  /* reset all entries */
  for (i=0; i<TOTAL_PAGES; i++)
    {
      pagusr_table[i].entry = 0;
    }
  /* Init kernel pages */
  for (i=0; i<NUM_PAG_KERNEL; i++)
    {
      // Logical page equal to physical page (frame)
      pagusr_table[i].bits.pbase_addr = i;
      pagusr_table[i].bits.rw = 1;
      pagusr_table[i].bits.present = 1;
    }
  /* Protect task_structs */
  for (i=0; i<NR_TASKS; i++)
    {
      
      page = ((DWord)&task[i].task_protect>>12); //Get the page to protect.
      pagusr_table[page].bits.pbase_addr = page;
      pagusr_table[page].bits.rw = 0; //Disable write access to protected page
      pagusr_table[page].bits.present = 0; /* Generate a Page Fault when accessed */
    }
}

/* Initialize pages for initial process (user pages) */
void set_user_pages( void )
{
 int pag; 
 int first_ph_page = NUM_PAG_KERNEL;

  /* CODE */
  for (pag=PAG_LOG_INIT_CODE_P0;pag<PAG_LOG_INIT_DATA_P0;pag++){
  	pagusr_table[pag].entry = 0;
  	pagusr_table[pag].bits.pbase_addr = first_ph_page;
  	pagusr_table[pag].bits.user = 1;
  	pagusr_table[pag].bits.present = 1;
        first_ph_page++;
  }
  
  /* DATA */ 
  for (pag=PAG_LOG_INIT_DATA_P0;pag<PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA;pag++){
  	pagusr_table[pag].entry = 0;
  	pagusr_table[pag].bits.pbase_addr = alloc_frame();
  	pagusr_table[pag].bits.user = 1;
  	pagusr_table[pag].bits.rw = 1;
  	pagusr_table[pag].bits.present = 1;
  }
}

/* Writes on CR3 register producing a TLB flush */
void set_cr3()
{
 	asm volatile("movl %0,%%cr3": :"r" (dir_pages));
}

/* Macros for reading/writing the CR0 register, where is shown the paging status */
#define read_cr0() ({ \
         unsigned int __dummy; \
         __asm__( \
                 "movl %%cr0,%0\n\t" \
                 :"=r" (__dummy)); \
         __dummy; \
})
#define write_cr0(x) \
         __asm__("movl %0,%%cr0": :"r" (x));
         
/* Enable paging, modifying the CR0 register */
void set_pe_flag()
{
  unsigned int cr0 = read_cr0();
  cr0 |= 0x80000000;
  write_cr0(cr0);
}

/* Associates logical page 'page' with physical page 'frame' */
void set_ss_pag(unsigned page,unsigned frame)
{
	pagusr_table[page].entry=0;
	pagusr_table[page].bits.pbase_addr=frame;
	pagusr_table[page].bits.user=1;
	pagusr_table[page].bits.rw=1;
	pagusr_table[page].bits.present=1;

}
/* Initializes paging for the system address space */
void init_mm()
{
  init_table_pages();
  init_frames();
  init_dir_pages();
  set_cr3();
  set_pe_flag();
}
/***********************************************/
/************** SEGMENTATION MANAGEMENT ********/
/***********************************************/
void setGdt()
{
  /* Configure TSS base address, that wasn't initialized */
  gdt[KERNEL_TSS>>3].lowBase = lowWord((DWord)&(tss));
  gdt[KERNEL_TSS>>3].midBase  = midByte((DWord)&(tss));
  gdt[KERNEL_TSS>>3].highBase = highByte((DWord)&(tss));

  gdtR.base = (DWord)gdt;
  gdtR.limit = 256 * sizeof(Descriptor);

  set_gdt_reg(&gdtR);
}

/***********************************************/
/************* TSS MANAGEMENT*******************/
/***********************************************/
void setTSS()
{
  tss.PreviousTaskLink   = NULL;
  tss.esp0               = KERNEL_ESP;
  tss.ss0                = __KERNEL_DS;
  tss.esp1               = NULL;
  tss.ss1                = NULL;
  tss.esp2               = NULL;
  tss.ss2                = NULL;
  tss.cr3                = NULL;
  tss.eip                = 0;
  tss.eFlags             = INITIAL_EFLAGS; /* Enable interrupts */
  tss.eax                = NULL;
  tss.ecx                = NULL;
  tss.edx                = NULL;
  tss.ebx                = NULL;
  tss.esp                = USER_ESP;
  tss.ebp                = tss.esp;
  tss.esi                = NULL;
  tss.edi                = NULL;
  tss.es                 = __USER_DS;
  tss.cs                 = __USER_CS;
  tss.ss                 = __USER_DS;
  tss.ds                 = __USER_DS;
  tss.fs                 = NULL;
  tss.gs                 = NULL;
  tss.LDTSegmentSelector = KERNEL_TSS;
  tss.debugTrap          = 0;
  tss.IOMapBaseAddress   = NULL;

  set_task_reg(KERNEL_TSS);
}

 
/* Initializes the ByteMap of free physical pages.
 * The kernel pages are marked as used */
int init_frames( void )
{
    int i;
    /* Mark pages as Free */
    for (i=0; i<TOTAL_PAGES; i++) {
        phys_mem[i] = FREE_FRAME;
    }
    /* Mark kernel pages as Used */
    for (i=0; i<NUM_PAG_KERNEL; i++) {
        phys_mem[i] = USED_FRAME;
    }
    return 0;
}

/* initialize_P0_frames - Initializes user code frames and user data frames for Initial Process P0.
 * Returns the First Physical frame for P0 */
int initialize_P0_frames(void)
{
    int i;
    /* Mark User Code pages as Used */
    for (i=0; i<NUM_PAG_CODE; i++) {
        phys_mem[NUM_PAG_KERNEL+i] = USED_FRAME;
    }
    return NUM_PAG_KERNEL;
}
/* alloc_frame - Search a free physical page (== frame) and mark it as USED_FRAME. 
 * Returns the frame number or -1 if there isn't any frame available. */
int alloc_frame( void )
{
    int i;
    for (i=NUM_PAG_KERNEL; i<TOTAL_PAGES;) {
        if (phys_mem[i] == FREE_FRAME) {
            phys_mem[i] = USED_FRAME;
            return i;
        }
        i += 2; /* NOTE: There will be holes! */
    }
    
    return -1;
}

/* free_frame - Mark as FREE_FRAME the frame  'frame'.*/
void free_frame( unsigned int frame )
{
    phys_mem[frame] = FREE_FRAME;
}


