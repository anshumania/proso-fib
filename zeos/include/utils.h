#ifndef UTILS_H
#define UTILS_H

void copy_data(void *start, void *dest, int size);
int copy_from_user(void *start, void *dest, int size);
int copy_to_user(void *start, void *dest, int size);
void itoa(int num, char *buffer);
int access_ok(int type, const void *addr, unsigned long size);
int strlen(const char *buffer);
int strcmp (const char *s1, const char *s2);
int check_address(const void *addr, int size);

#endif
