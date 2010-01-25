#include <libc.h>
#include <stdio.h>

int strlen(const char *buffer){
	int i=0;
	for(;buffer[i]!=0;i++){}

	return i;
}

void itoa(int num, char *buffer){
	int digits = num;
	int i = 0;

	while(digits > 0){
		i++;
		digits /= 10;
	}

	while (i > 0){
		buffer[i-1]=(num%10)+'0';
		num /= 10;
		i--;
	}
}

/*Strcmp implementation, thanks to:
 * P.J. Plauger, The Standard C Library, 1992
 */
int strcmp (const char *s1, const char *s2){
  for (; *s1 == *s2; ++s1, ++s2)
    if (*s1 == 0)
      return 0;
  return *(unsigned char *) s1 < *(unsigned char *) s2 ? -1 : 1;
}
