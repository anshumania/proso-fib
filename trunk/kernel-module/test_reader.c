#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

struct t_info{
  int n_calls;
  int n_calls_ok;
  int n_calls_err;
  unsigned long long total;
};

int monitored_pid;
int monitored_sysc;

#define CH_PID 0
#define CH_SYSC 1
#define RESET_VALS 2
#define RESET_ALL 3
#define ACT_SYSC 4
#define DEACT_SYSC 5

void print_stats (int call_index,struct t_info *th_stats){
  char call[7];

  switch(call_index){
	  case 0:
		  sprintf(call,"OPEN");
		  break;
	  case 1:
		  sprintf(call,"CLOSE");
		  break;
	  case 2:
		  sprintf(call,"WRITE");
		  break;
	  case 3:
		  sprintf(call,"CLONE");
		  break;
	  case 4:
		  sprintf(call,"LSEEK");
		  break;
  }

  printf ("Statistics for the %s system call:\n",call);
  printf ("------------------------------------\n");
  printf ("PID          : %i\n", monitored_pid);
  printf ("# Calls      : %i\n", th_stats->n_calls);
  printf ("# OKs        : %i\n", th_stats->n_calls_ok);
  printf ("# ERRs       : %i\n", th_stats->n_calls_err);
  printf ("Total time   : %lld\n", th_stats->total);
}

int main (){
  int fd, fd2, result,vfork;
  char buffer[50];
  struct t_info stats;
  char err[50];

  monitored_pid = getpid();

  /* To make the following work, these 2 lines have to be added to /lib/modules/$(uname -r)/modules.dep
   *
   * /home/alumne/Desktop/ProSO/Project2/syscalls_mon.ko:
   * /home/alumne/Desktop/ProSO/Project2/syscalls_mon_reader.ko: /home/alumne/Desktop/ProSO/Project2/syscalls_mon.ko
   *
   * These will allow modprobe to know which modules to load.
   */
  system("modprobe syscalls_mon_reader");

  /* Creates the new device (major 253, minor 0) */
  system ("mknod /dev/syscalls_stats_reader c 253 0\n");
  printf ("\n[OPEN] Opening the device: ");
  /* Opens the device just created */
  fd = open ("/dev/syscalls_stats_reader", O_RDONLY);
  if (fd < 0)
    goto msg_error;

  printf ("Done\n[OPEN] Trying to open the device again: ");
  fd2 = open ("/dev/syscalls_stats_reader", O_RDONLY);
  if (fd2 >= 0)
    goto msg_error;

  printf ("Done\n[READ] Reading data for this process:\n");
  result = read (fd, &stats, sizeof (struct t_info));
  if (result < 0)
    goto msg_error;

  print_stats (monitored_sysc,&stats);

  /* This is for testing the ability to change the PID.
   * We create a child and use its pid to run the test.*/

  vfork= fork();
  if(vfork==0){
	  monitored_pid = getpid ();
	  printf ("Done\n[IOCTL] Changing PID to %d: ", monitored_pid);
	  result = ioctl (fd, CH_PID, monitored_pid);
	  if (result < 0)
		  goto msg_error;

	  printf ("Done\n[READ] Reading data for this process:\n");
	  result = read (fd, &stats, sizeof (struct t_info));
	  if (result < 0)
		  goto msg_error;

	  print_stats (monitored_sysc,&stats);

	  exit(0);
  }
  waitpid (vfork, NULL, 0); //Waits for the child to be dead

  monitored_pid=getpid();
  printf ("Done\n[IOCTL] Going back to PID %d (father): ", monitored_pid);
  result = ioctl (fd, CH_PID, monitored_pid);
  if (result < 0)
	  goto msg_error;
  /* End of test for CH_PID */


  printf ("Done\n[IOCTL] Changing system call to WRITE: ");
  result = ioctl (fd, CH_SYSC, 2);
  if (result < 0)
    goto msg_error;

  monitored_sysc=2;

  printf ("Done\n[READ] Reading data from PID %i:\n", monitored_pid);
  result = read (fd, &stats, sizeof (struct t_info));

  if (result < 0)
    goto msg_error;
  print_stats (monitored_sysc,&stats);

  printf ("Done\n[IOCTL] Reseting statistics for PID %i: ", monitored_pid);
  result = ioctl (fd, RESET_VALS, monitored_pid);
  if (result < 0)
    goto msg_error;

  printf ("Done\n[READ] Reading reset statistics:\n");
  result = read (fd, &stats, sizeof (struct t_info));
  if (result < 0)
    goto msg_error;

  print_stats (monitored_sysc,&stats);

  printf ("Done\n[IOCTL] Reseting the statistics of every process: ");
  result = ioctl (fd, RESET_ALL, 0);
  if (result < 0)
    goto msg_error;

  printf ("Done\n[READ] Reading reset statistics:\n");
  result = read (fd, &stats, sizeof (struct t_info));
  if (result < 0)
	  goto msg_error;

  print_stats (monitored_sysc,&stats);

  printf ("Done\n[IOCTL] Disable all the system calls: ");
  result = ioctl (fd, DEACT_SYSC, -1);
  if (result < 0)
    goto msg_error;

  printf("This is not going to be counted. Next statistics should indicate 9 WRITE calls, and they are from the previous print_stats().\n");

  printf("[READ] Reprint statistics after a dumb write to check if statistics changed:\n");
  result = read (fd, &stats, sizeof (struct t_info));
  if (result < 0)
	  goto msg_error;

  print_stats (monitored_sysc,&stats);

  printf ("Done\n[IOCTL] Activating all the system calls: ");
  result = ioctl (fd, ACT_SYSC, -1);
  if (result < 0)
    goto msg_error;

  printf ("Done\n[READ] Reading statistics:\n");
  result = read (fd, &stats, sizeof (struct t_info));
  if (result < 0)
  	  goto msg_error;

  print_stats (monitored_sysc,&stats);

  printf ("Done\n[RELEASE] Closing the device: ");
  result = close (fd);
  if (result < 0)
    goto msg_error;

  printf ("Done\n\nTEST COMPLETED SUCCESFULLY!\n");

  system("modprobe -r syscalls_mon_reader");

  exit (0);
msg_error:
  perror(err);
  printf ("\nERROR: %s\nCaution: The modules could still be loaded!!\nTerminating execution...\n",err);
  exit (0);
}
