/*
 * io.h - Definició de l'entrada/sortida per pantalla en mode sistema
 */

#ifndef __IO_H__
#define __IO_H__

#include <types.h>

/** Screen functions **/
/**********************/

Byte inb (unsigned short port);
void printc(char c);
void printk(char *string);

void printk_xy (int x, int y, char *string);
void printc_xy (int x, int y, char c);

void clean_line (int y);

#endif  /* __IO_H__ */
