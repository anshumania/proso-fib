#include <filesystem.h>
#include <sys.h>
#include <utils.h>
#include <devices.h>
#include <errno.h>

void init_dir (){
  int i;

  for (i = 0; i < MAX_FILES; i++)
    {
	  dir[i].n_refs = -1;
      ops[i].sys_open_dev = NULL;
      ops[i].sys_close_dev = NULL;
      ops[i].sys_read_dev = NULL;
      ops[i].sys_write_dev = NULL;
      ops[i].sys_release_dev = NULL;
      dir[i].operations = &ops[i];
    }

  copy_data ("KEYBOARD", dir[0].name, 8);
  dir[0].access_mode_allow = O_RDONLY;
  dir[0].operations->sys_read_dev = sys_read_keyboard;
  dir[0].size = NULL;
  dir[0].n_refs = 0;

  copy_data ("DISPLAY", dir[1].name, 7);
  dir[1].access_mode_allow = O_WRONLY;
  dir[1].operations->sys_write_dev = sys_write_console;
  dir[1].size = NULL;
  dir[1].n_refs = 0;
}

void init_fdt(){
	int i;
	for (i = 0; i < MAX_OPEN_FILES; i++){
		fdt[i].refs = 0;
		fdt[i].access_mode = 0;
		fdt[i].offset = 0;
		fdt[i].logic_dev = NULL;
	}
}

void initZeosFat (){
  int i;
  ffb = 0;

  for (i = 0; i < MAX_BLOCKS - 1; i++) fat[i] = i + 1;

  fat[MAX_BLOCKS - 1] = -1;
}

void init_disk (){
  int i, j;
  for (i = 0; i < MAX_BLOCKS; i++)
    for (j = 0; j < BLOCK_SIZE; j++)
      disk[i][j] = 0;
}

void init_filesystem (){
  init_dir ();
  init_fdt ();
  init_disk ();
  initZeosFat ();
}

/* Returns the first block of a list of nblocks free chained blocks.*/
int balloc (int nblocks){
  int tot, first, prev;

  /* Check whether the parameter is in the range and there is any
   * free block.*/
  if (ffb == -1 || nblocks < 1)
    return -1;

  /* If the first free block is the only block in the fat
   * check how many blocks are requested.
   * If it's more then 1, then return error.*/
  if (fat[ffb] == -1){
      if (nblocks == 1){
          prev = ffb;
          ffb = -1;
          return prev;
        }
      return -1;
    }

  /* Gets the first free block
   * and sets all the variables needed in case a
   * rollback will be needed.*/
  prev = ffb;
  first = ffb;
  ffb = fat[ffb];
  tot = 1;

  /* Loops for every requested block or until there are no
   * free blocks left. */
  while (tot != nblocks && fat[ffb] != -1){
      prev = ffb;
      ffb = fat[ffb];
      tot++;
    }

  /* If the last block of the last loop is
   * the last free block of the fat...*/
  if (fat[ffb] == -1){
	  /* ...and what is left to fulfill the request is one block...*/
      if (nblocks - tot == 1){ /* ... assign this last block.*/
          prev = ffb;
          ffb = -1;
        }
      else{
          if (nblocks - tot > 1){
        	  /* if there are more pending blocks,
			     means there is not enough free space.
				 rollback and return error. */
        	  ffb = first;
              return -1;
            }
        }
    }

  /* Terminate the list of block assigned changing the value
   * in the last block of the list.*/
  fat[prev] = -1;

  /* Returns the first block of the list.*/
  return first;
}

struct file *create_file (const char *path){
  int i, fblock, length;

  /* Finds the first free entry in the Directory.
   * If there is no one, returns error.*/
  for (i = 0; dir[i].n_refs != -1 && i < MAX_FILES; i++);
  if (i == MAX_FILES)
	 return (struct file *) -ENFILE;

  /* Checks the file name for its length*/
  length = strlen (path);

  if (length > MAX_NAME_LENGTH -1) //  - 1 is for the '\0' which we'add
    return (struct file *) -ENAMETOOLONG;


  /* Allocate one free initial block for the file.
   * If there is none, returns error.*/
  fblock = balloc (1);
  if (fblock < 0)
    return (struct file *) -ENOSPC;

  copy_from_user ((char *) path, dir[i].name, length);
  if(dir[i].name[length]!='\0') dir[i].name[length]='\0';
  dir[i].access_mode_allow = O_RDWR;
  dir[i].operations->sys_open_dev = NULL;
  dir[i].operations->sys_close_dev = NULL;
  dir[i].operations->sys_read_dev = sys_read_file;
  dir[i].operations->sys_write_dev = sys_write_file;
  dir[i].operations->sys_release_dev = sys_release_file;
  dir[i].first_block = fblock;
  dir[i].size = 0;
  dir[i].n_blocks = 1;
  dir[i].n_refs = 0;
  return &dir[i];
}

/*
 * Frees a block used by a file.
 * Note: this function DOESN'T control if index points to a already free block
 * and will take index as the first block of the file.
 */
int freeb (int index){
  int i;

  /* Checks the parameter*/
  if (index >= MAX_BLOCKS || index < 0 || index == ffb)
    return -1;

  /* If there were no free blocks, set ffb to this block now.*/
  if (ffb == -1){
      ffb = index;
      return 0;
    }

  /* Adds the newly freed block to the list of free blocks.*/
  for (i = ffb; fat[i] != -1; i=fat[i]);

  fat[i] = index;

  return 0;
}

void add_block (int last, int new){
  fat[last] = new;
  fat[new] = -1;
}
