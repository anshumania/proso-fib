#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sched.h>

//#define printOk(...) { printf(" Ok!\n"); }

//#include <stdlib.h>

int dumb_funct(){
	int i;
	for(i=0;i<10;i++);

	return 0;
}

int main (){
  pid_t pid;
  int fd,p;
  char c;
  char buffer[] = "Non è tutto oro quello che luccica.\n\0";

  pid = getpid ();
  printf ("Starting up the test...\n");
  printf ("We will use PID: %i\nPress any key to proceed...\n", pid);
  getchar();

  char arg[50];
  int uid;
  uid = getuid ();
  if (uid != 0)
	exit (2);

  sprintf (arg, "insmod syscalls_mon.ko pid_init=%d", pid);
  system(arg);

  printf("The module has been successfully loaded.\nPress any key to begin the tests...\n");
  getchar();

  fd = open ("del_tmp", O_RDWR | O_CREAT);
  if (fd < 0)
    goto error;

  if (open ("nonexistent", O_RDWR) >= 0)
    goto error;

  if (write (fd, &buffer,37) < 0)
    goto error;

  if (write (-1, &buffer, -1) >= 0)
    goto error;

  if (lseek (fd, 1, SEEK_SET) < 0)
    goto error;

  if (lseek (-1, 1, SEEK_CUR) >= 0)
    goto error;

  p=fork();
  if(p==0) exit(0);

  /*This is for the test of wrong clone.
   * I noticed that the clone doesn't enter to sys_clone if at list the two
   * first parameters are acceptable.
   * So we use dub_funct() as an int dummy function and stack as a memory location.
   * The error is in the flags: CLONE_SIGHAND cannot be used without CLONE_VM, and this generates EINVAL. */
  void* stack;
  stack=malloc(4096); // These creates a stack for the child of the clone.

  if (clone(dumb_funct, stack, CLONE_SIGHAND ,NULL) >= 0)
    goto error;

  if (close (fd) < 0)
    goto error;

  if (close (-1) >= 0)
    goto error;

  system("rmmod syscalls_mon");

  printf ("The module has been unloaded succesfully.\nAll tests were successful.\n");
  printf ("\nFor further informations on the execution of the module, check:\n/var/log/messages\n/var/log/kern.log\n\n\n");

  printf("End of test program execution.\nPress any key to exit...");
  getchar();

  // We delete the temporary file we created before
  system("rm -f del_tmp");

  exit (0);

error:
  printf ("\nProblem detected, terminating execution...\n");
  system("rm -f del_tmp");
  exit (0);
}
