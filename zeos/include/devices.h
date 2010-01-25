#ifndef DEVICES_H__
#define  DEVICES_H__

#include <sched.h>

int sys_write_console(int fd, char *buffer,int size);
void copy_from_circ_buffer(struct task_struct *blocked, int size);
void unblock_and_enqueque (struct task_struct *task);
void block_into_kbd (struct task_struct *task);
void step_circ_buffer ();
int sys_read_keyboard (int fd, char *buffer, int size);
void init_keyboard();

int sys_release_file (int entry);
int sys_write_file (int fd, char *buffer, int size);
int sys_read_file (int fd, char *buffer, int size);

#endif /* DEVICES_H__*/
