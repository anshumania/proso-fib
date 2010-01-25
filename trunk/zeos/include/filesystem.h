
#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <types.h>
#include <sched.h>

#define O_RDONLY             0x1
#define O_WRONLY             0x2
#define O_RDWR               0x3
#define O_CREAT              0x4
#define O_EXCL               0x8

/* Disk will be 80*256bytes big: 20KB */
#define MAX_BLOCKS 80
#define BLOCK_SIZE 256
#define MAX_FILES 10
#define MAX_NAME_LENGTH 10
#define MAX_OPEN_FILES 15

/* ZeOSFat */
int fat[MAX_BLOCKS];

/* ffb: First Free Block.
 * Contains a pointer to the first free
 * block into the fat.*/
int ffb;

/* Ram-Disk */
Byte disk[MAX_BLOCKS][BLOCK_SIZE];


/* Directory is an array of files which is the structure
 * to keep track of the Logical Devices.
 * If n_refs=-1 then the file is invalid.*/
struct file{
  char name[MAX_NAME_LENGTH];
  int access_mode_allow;
  struct file_operations * operations;
  int first_block;
  int n_blocks;
  int size;
  int n_refs;
} dir[MAX_FILES];

/* Structure which contains the pointers
 * to device dependent routines.*/
struct file_operations{
  int (*sys_open_dev)(const char *, int);
  int (*sys_close_dev)(int);
  int (*sys_read_dev)(int, char*, int);
  int (*sys_write_dev)(int, char*, int);
  int (*sys_release_dev) (int dir_entry);
} ops[MAX_FILES];


/* File Descriptor Table: it contains information
   about all the virtual devices open in the system.*/
struct virtual {
  int access_mode;
  int refs;
  int offset;
  struct file * logic_dev;
} fdt[MAX_OPEN_FILES];



void initZeosFat();
void initdisk();
void init_dir(void);
void init_fdt(void);
void init_filesytem();
struct file *create_file (const char *path);
int balloc (int nblocks);
int freeb(int index);
void add_block (int last, int new);

#endif /* FILESYSTEM_H_ */
