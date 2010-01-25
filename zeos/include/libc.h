/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */
 
#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>

int write(int fd,char *buffer,int size);
int getpid(void);
int fork(void);
int sem_init(int n_sem, unsigned int value);
int sem_wait(int n_sem);
int sem_signal(int n_sem);
int sem_destroy(int n_sem);
void exit();
int nice (int quantum);
int get_stats (int pid, struct stats *st);
int check_errcode(int code);
void perror();
int dup (int fd);
int close (int fd);
int read (int fd, char *buffer, int size);
int open (const char *path, int flags);
int unlink (const char *path);

#endif  /* __LIBC_H__ */
