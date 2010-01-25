/*
 * libc.c 
 */
#include <libc.h>
#include <stdio.h>

int errno = 0; /* variable which contains the code of the error. */

/* Wrapper of  write system call*/
int write(int fd,char *buffer,int size)
{
	int val;
	/*
		Assembly instructions for using the stack to pass parameters and then generate the 		sofware interrupt. The return value will be retrieved by %eax into "val"
	*/
	__asm__ __volatile__("movl 8(%%ebp), %%ebx\n" "movl 12(%%ebp), %%ecx\n" "movl 16(%%ebp), %%edx\n" "movl $4, %%eax\n" "int $0x80\n" "movl %%eax, %0\n":"=g" (val)::"bx");
	
	/* At this point "val" contains the output value of the syscall.
	   We have to check it and, if negative, set errno with the absolute value of val and
	   return -1 */
	
	return check_errcode(val);
}

/* Wrapper of getpid syscall */
int getpid(void){
	int pid;
	
	__asm__ __volatile__ ("movl $20, %%eax\n"
						  "int $0x80\n"
						  "movl %%eax,%0\n":"=g" (pid)::"bx");
	
	return pid;
}

/* Wrapper of fork syscall */
int fork(void){
	int val=-1;
	
	__asm__ __volatile__ ("movl $2, %%eax\n"
						  "int $0x80\n"
						  "movl %%eax,%0\n":"=g" (val)::"bx");
	
	return check_errcode(val);
}

int check_errcode(int code){
	if(code<0){
		errno=-code;
		return -1;
	}
	return code;
}

void perror(){
	char *buffer;

	switch(errno){
		case 1:
			buffer="EPERM: Operation not permitted\n\0";
			break;
		case 3:
			buffer="ESRCH: No such process\n\0";
			break;
		case 4:
			buffer="EINTR: Interrupted function call\n\0";
			break;
		case 9:
			buffer="EBADF: Bad file descriptor\n\0";
			break;
		case 11:
			buffer="EAGAIN: Resource busy, try again\n\0";
			break;
		case 13:
			buffer="EACCES: Permission denied\n\0";
			break;
		case 14:
			buffer="EFAULT: Bad address\n\0";
			break;
		case 16:
			buffer="EBUSY: Device or resource busy\n\0";
			break;
		case 22:
			buffer="EINVAL: Invalid argument\n\0";
			break;
		case 23:
			buffer="ENFILE: Too many open files in system\n\0";
			break;
		case 24:
			buffer="EMFILE: Too many open files by the process\n\0";
			break;
		case 27:
			buffer="EFBIG: File too large\n\0";
			break;
		case 28:
			buffer="ENOSPC: No space left on device\n\0";
			break;
		case 36:
			buffer="ENAMETOOLONG: File name too long\n\0";
			break;
		case 38:
			buffer="ENOSYS: Function not implemented\n\0";
			break;
		case 44:
			buffer="ECHRNG: Channel number out of range\n\0";
			break;
		default:
			buffer="Unknown error or no error\n\0";
			break;
	}

	write(1,buffer,strlen(buffer));
}

int sem_init(int n_sem, unsigned int value){
	int val=-1;

	__asm__ __volatile__ ("movl 8(%%ebp), %%ebx\n"
						  "movl 12(%%ebp), %%ecx\n"
						  "movl $21, %%eax\n"
						  "int $0x80\n"
						  "movl %%eax,%0\n":"=g" (val)::"bx");

	return check_errcode(val);
}

int sem_wait(int n_sem){
	int val=-1;

	__asm__ __volatile__ ("movl 8(%%ebp), %%ebx\n"
						  "movl $22, %%eax\n"
						  "int $0x80\n"
						  "movl %%eax,%0\n":"=g" (val)::"bx");

	return check_errcode(val);
}

int sem_signal(int n_sem){
	int val=-1;

	__asm__ __volatile__ ("movl 8(%%ebp), %%ebx\n"
						  "movl $23, %%eax\n"
						  "int $0x80\n"
						  "movl %%eax,%0\n":"=g" (val)::"bx");

	return check_errcode(val);
}

int sem_destroy(int n_sem){
	int val=-1;

	__asm__ __volatile__ ("movl 8(%%ebp), %%ebx\n"
						  "movl $24, %%eax\n"
						  "int $0x80\n"
						  "movl %%eax,%0\n":"=g" (val)::"bx");

	return check_errcode(val);
}

/* This call terminates the process which has called it.
 * Obviously it will NEVER return.*/
void exit(){
	__asm__ __volatile__ ("movl $1, %%eax\n"
						  "int $0x80\n"
						::);
}

/* This system call modifies the value of the quantum of the process which call it.
 * It returns the former quantum value if everything went fine, -1 otherwise. */
int nice (int quantum){
	int val = -1;

	__asm__ __volatile__ ("movl 8(%%ebp), %%ebx\n"
						  "movl $34, %%eax\n"
						  "int $0x80\n"
						  "movl %%eax,%0\n":"=g" (val)::"bx");

	return check_errcode (val);
}

int get_stats (int pid, struct stats *st){
	int val = -1;
	__asm__ __volatile__ ("movl 8(%%ebp),%%ebx\n"
						"movl 12(%%ebp),%%ecx\n"
						"movl $35,%%eax\n"
						"int $0x80\n"
						"movl %%eax, %0\n":"=g" (val)
						::"bx");

	return check_errcode (val);
}

int open (const char *path, int flags){
	int val = -1;
	__asm__ __volatile__ ("movl 8(%%ebp),%%ebx\n"
						"movl 12(%%ebp),%%ecx\n"
						"movl $5,%%eax\n"
						"int $0x80\n"
						"movl %%eax, %0\n":"=g" (val)::"%ebx");
	return check_errcode (val);
}

int read (int fd, char *buffer, int size){
	int val = -1;
	__asm__ __volatile__ ("movl 8(%%ebp),%%ebx\n"
						  "movl 12(%%ebp),%%ecx\n"
						  "movl 16(%%ebp),%%edx\n"
						  "movl $3,%%eax\n"
						  "int $0x80\n"
						  "movl %%eax, %0\n":"=g" (val)
						::"%ebx");
	return check_errcode (val);
}

int close (int fd){
  int val = -1;
  __asm__ __volatile__ ("movl 8(%%ebp),%%ebx\n"
                        "movl $6,%%eax\n"
                        "int $0x80\n"
						"movl %%eax,%0\n":"=g" (val)::"%ebx");
  return check_errcode (val);
}

int dup (int fd){
  int val = -1;
  __asm__ __volatile__ ("movl 8(%%ebp),%%ebx\n"
                        "movl $41,%%eax\n"
                        "int $0x80\n"
						"movl %%eax,%0\n":"=g" (val)::"%ebx");
  return check_errcode (val);
}

int unlink (const char *path){
  int val = -1;
  __asm__ __volatile__ ("movl 8(%%ebp),%%ebx\n"
                        "movl $10,%%eax\n"
                        "int $0x80\n"
						"movl %%eax,%0\n":"=g" (val)::"%ebx");
  return check_errcode (val);
}
