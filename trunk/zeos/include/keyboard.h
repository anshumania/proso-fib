#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__


#define CIRCULAR_SIZE 5

#include <list.h>

// Queue of blocked process, waiting for some characters from the keyboard
// A process will be blocked in this queue until all the characters requested are available
struct list_head keyboard_queue;

char circular[CIRCULAR_SIZE];
// Number of characters into the circular buffer
int circ_chars;
int p;
int start;

#endif /* __KEYBOARD_H__ */
