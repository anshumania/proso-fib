#ifndef _I386_ERRNO_H
#define _I386_ERRNO_H

#define EPERM			1		/* Operation not permitted */
#define ENOENT          2       /* No such file or directory */
#define	ESRCH			3		/* No such process */
#define EINTR			4		/* Interrupted function call */
#define EBADF			9		/* Bad file descriptor */
#define EAGAIN			11		/* Resource busy, try again */
#define	EACCES			13		/* Permission denied */
#define EFAULT			14		/* Bad address */
#define	EBUSY			16		/* Device or resource busy */
#define	EEXIST			17		/* File exists */
#define EINVAL			22		/* Invalid argument */
#define ENFILE          23      /* Too many open files in system */
#define EMFILE          24      /* Too many open files by the process */
#define EFBIG           27      /* File too large */
#define	ENOSPC			28		/* No space left on device */
#define ENAMETOOLONG    36      /* File name too long */
#define ENOSYS			38		/* Function not implemented */
#define ECHRNG          44      /* Channel number out of range */

#endif
