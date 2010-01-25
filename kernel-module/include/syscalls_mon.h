//#include <sys/syscall.h>

#ifndef SYSCALLS_MON_H_
#define SYSCALLS_MON_H_

#define N_CALLS 5

#define OPEN    0
#define CLOSE   1
#define MYWRITE 2
#define CLONE   3
#define LSEEK   4

extern void *sys_call_table[];
void *sys_call_table_original[N_CALLS];
extern void *sys_call_table[];
void *sys_call_table_original[N_CALLS];
void *sys_call_table_local[N_CALLS];


/* proso_get_cycles: function used to count execution time of syscalls */
#define proso_rdtsc(low,high) \
  __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

static inline unsigned long long proso_get_cycles (void){
  unsigned long eax, edx;

  proso_rdtsc (eax, edx);
  return ((unsigned long long) edx << 32) + eax;
}

struct t_info{
  int n_calls;
  int n_calls_ok;
  int n_calls_err;
  unsigned long long total;
};

/* This structure will contain informations about every the syscall for ALL the processes. */
struct t_info syscall_info_table[N_CALLS];

struct my_thread{
  /* default thread info structure, which now we put into our structure.*/
  struct thread_info info_th;
  /* The reason why there's a +1 in this array is because at that extra entry we'll
   * put global data for all the syscalls of the process. */
  struct t_info stats[N_CALLS + 1];
  /* Next field is used to check validity of data contained by struct: if it contains
   * a consistent pid, then statistics are coherent and valid. */
  int pid;
};

/* Table which correlates syscall indexes given internally the module with system known indexes.
 * It will be used to make easier and more linear activation/deactivation of single syscalls.*/
int local_to_sys_index[]={__NR_open,__NR_close,__NR_write,__NR_clone,__NR_lseek};

/* Local traps */
long sys_open_local (const char /*__user*/ * filename, int flags, int mode);
long sys_close_local (unsigned int fd);
ssize_t sys_write_local (unsigned int fd, const char /*__user*/ * buf, size_t count);
int sys_clone_local (struct pt_regs regs);
off_t sys_lseek_local (unsigned int fd, off_t offset, unsigned int origin);
/***************/

/* Functions to be used by outside of the module */
void set_mon_on_syscall (int call_index);
void unset_mon_on_syscall (int call_index);
void reset_stats (int pid, struct my_thread *t_info_stats);
void print_stats (int pid);
void print_total_sys_stats(void);
int get_stats (int pid, int call, struct t_info *stats);
/*************************************************/

#endif /* SYSCALLS_MON_H_ */
