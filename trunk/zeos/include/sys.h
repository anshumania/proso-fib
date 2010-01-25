#ifndef __SYS_H__
#define __SYS_H__

#include <stats.h>

int sys_ni_syscall();
int sys_write(int fd, char *buffer, int size);
int sys_getpid(void);
int sys_fork(void);
int sys_sem_init (int n_sem, unsigned int value);
int sys_sem_wait (int n_sem);
int sys_sem_signal (int n_sem);
int sys_sem_destroy (int n_sem);
void sys_exit();
int sys_nice (int quantum);
int sys_get_stats (int spid, struct stats *st);
int sys_close (int fd);
int sys_dup (int fd);
int sys_read (int fd, char *buffer, int size);
int sys_open (const char *path, int flags);
int sys_unlink (const char *path);

#endif
