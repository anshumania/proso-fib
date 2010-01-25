#include <devices.h>
#include <keyboard.h>
#include <io.h>
#include <sched.h>
#include <mm.h>
#include <errno.h>
#include<utils.h>

#define MAX_WRITE_BLOCK 256

int sys_write_console(int fd, char *buffer, int size){

	char k_buffer[MAX_WRITE_BLOCK];
	int out_block,loop,i,j,return_write=0;

	loop=size/MAX_WRITE_BLOCK;
	out_block=size%MAX_WRITE_BLOCK;

	for(i=0;i<loop;i++){
		copy_from_user(buffer+(i*MAX_WRITE_BLOCK),k_buffer,MAX_WRITE_BLOCK);
		for(j=0;j<MAX_WRITE_BLOCK;j++)
			printc(k_buffer[j]);
		return_write+=MAX_WRITE_BLOCK;
	}

	if(out_block!=0){
		copy_from_user(buffer+(size-out_block),k_buffer,out_block);
		for(j=0;j<out_block;j++)
			printc(k_buffer[j]);
		return_write+=out_block;
	}

	return return_write;
}

void init_keyboard(){
	p=0;
	start=0;
	circ_chars=0;
}

/* This function is the specific routine called for reading from the keyboard.
 * This kind of reading is done by means of a circular buffer which stores an amount of datas and returns
 * them to the processes which want them. */
int sys_read_keyboard (int fd, char *buffer, int size){
  union task_union *next_proc;
  int read;
  struct task_struct *proc = current();

  // Sets the number of chars that the process still needs to get to complete the call
  if(size>circ_chars)
	  proc->pending_chars = (size - circ_chars);
  else
	  proc->pending_chars = 0;

  if (proc->pending_chars <= 0 && list_empty (&keyboard_queue)){
      /* If there are enough chars into the buffer and there is no process blocked waiting
       * for them. */

	  read=size;

	  /* Copies data from circular to user buffer*/
      copy_from_circ_buffer (proc, read);
      //step_circ_buffer ();
      circ_chars -= size ;           /* We update the number of character contained into the circular buffer */
      start = (start+size) % CIRCULAR_SIZE;

      return read;          /* We return the number of read characters */

    }
  else if (proc->pid != 0){
	  /* We have to block the process until all requested chars are available.
	   * Task0 cannot be blocked.*/
      start = p;

      /* Blocks the process into keyboard_queue */
      block_into_kbd (proc);

      /* Updates needed data before blocking the process */
      proc->size = size;

      /* We update the return value of the this read. */
      ((union task_union *) proc)->stack[KERNEL_STACK_SIZE - 10] = size;

      /* Switch to the next scheduled process */
      next_proc = (union task_union *) (list_head_to_task_struct (runqueue.next));
      it_is_int = 0;
      task_switch (next_proc);

    }

  return -EPERM;
}

/* This function simulates the behavior of a FIFO queue over the circ_buffer array,
 * using p as an index parameter. */
void step_circ_buffer (){
  if (p == CIRCULAR_SIZE - 1)
    p = 0;
  else
    p++;
}

void block_into_kbd (struct task_struct *task){
/* Moves task from the runqueue to the keyboard_queue, blocking it.
 * It will be unblocked by the keyboard interrupt routine, when the
 * requested character will be available */
  list_del (&(task->run_list));
  list_add_tail (&(task->run_list), &keyboard_queue);
}

void unblock_and_enqueque (struct task_struct *task){
/* Puts the task back into the runqueue from the keyboard_queue, where it was blocked */
  list_del (&(task->run_list));
  list_add_tail (&(task->run_list), &runqueue);
}

void copy_from_circ_buffer(struct task_struct *blocked, int size){

  int i, temp=size;
  char buff[CIRCULAR_SIZE];

  for (i = 0; i < NUM_PAG_DATA; i++){
	  /* Associates the physical pages of the blocked process to the
	   * logical address space of the current process, substituting current process
	   * page table entries.
	   * We don't need to flush the TLB, because we're just adding pages to the table.
	   **/
	  set_ss_pag (PAG_LOG_INIT_DATA_P0 + NUM_PAG_DATA + i, blocked->ph_pages[i]);
    }

  /* We modify the logical address of the buffer so that we can reach it
   * after adding physical pages of the blocked process AFTER the current process ones.*/
  blocked->buffer += NUM_PAG_DATA * PAGE_SIZE;

  for (i=0; temp > 0; temp--, i++)
      /* We take the division module CIRCULAR_SIZE because, even when 'start + i' would be
       * greater then 14, we have the correct index in circular buffer. */
	  buff[i] = circular[(start + i) % CIRCULAR_SIZE];

  /* Copies the chars from current to blocked process*/
  copy_to_user (buff, blocked->buffer, size);


  for (i = 0; i < NUM_PAG_DATA; i++){
      /* Restores logical address space */
      set_ss_pag (PAG_LOG_INIT_DATA_P0 + NUM_PAG_DATA + i, current()->ph_pages[i]);
    }
  set_cr3 ();

  /* We restore the pointer logical address.*/
  blocked->buffer -= NUM_PAG_DATA * PAGE_SIZE;

}

int sys_release_file (int entry){
  freeb (dir[entry].first_block);
  dir[entry].n_blocks = 0;
  dir[entry].size = 0;

  return 0;
}

int sys_write_file (int fd, char *buffer, int size){
  int destBlock, destByte, i, newBlock;

  struct task_struct *proc = current ();
  struct virtual *open_file = proc->file_descriptors[fd];
  struct file *source = proc->file_descriptors[fd]->logic_dev;

  //destBlock = (open_file->offset / BLOCK_SIZE) + source->first_block;
  destByte = open_file->offset % BLOCK_SIZE;

  for(destBlock=source->first_block;fat[destBlock]!=-1;destBlock=fat[destBlock]);

  for (i = 0; i < size; i++){
      /* Checks if a new block is needed */
      if (source->size % BLOCK_SIZE == 0 && source->size != 0){
          newBlock = balloc (1);
          if (newBlock == -1) return -ENOSPC;

          add_block (destBlock, newBlock);

          source->n_blocks++;

          destByte = 0;
          destBlock = newBlock;
        }
      /* Data copying */
      if (copy_from_user(&buffer[i],&disk[destBlock][destByte],1) != 0)
    	  return -EFAULT;

      /* If the incremented offset is bigger then the file size, then we added new bytes*/
      if(source->size<(++(open_file->offset))) source->size++;
      destByte++;
    }

  return i;
}

int sys_read_file (int fd, char *buffer, int size){
  int block0, byte0, i, nthBlock;

  struct task_struct *proc = current ();
  struct virtual *open_file = proc->file_descriptors[fd];
  struct file *source = proc->file_descriptors[fd]->logic_dev;

  /* gets the first block of the file*/
  block0 = source->first_block;
  /* calculates how many blocks from the beginning block have been read already */
  nthBlock = open_file->offset / BLOCK_SIZE;
  /* calculates the offset in the actual block */
  byte0 = open_file->offset % BLOCK_SIZE;

  /* we loop for nthBlock times, to get to the block
   * which is being read right now*/
  for (i = 0; i < nthBlock; i++)
	  block0 = fat[block0];

  /* We'll read size byte of block0 beginning from byte0*/
  for (i = 0; i < size && open_file->offset < source->size; i++){
      /* Checks if we got to increment nthBlock */
      if (byte0 % BLOCK_SIZE == 0 && byte0 != 0){
          /* If the previous one was the last requested char or there isn't any
           * other block, just return i*/
    	  if ((open_file->offset == size)||(fat[block0] == -1))
    		  return i;

    	  /*if (fat[block0] == -1){
    		   We're at the end of the file.
    		   * Reset the pointer of the position and
    		   * return chars read till now.
    		  opened_file->offset = 0;
    		  return i;
    	  }*/

          /* the reading follows from next block of the file */
    	  block0 = fat[block0];
          byte0 = 0;
        }

      /* Data copy */
      buffer[i] = disk[block0][byte0];
      open_file->offset++;
      byte0++;
    }

  return i;
}
