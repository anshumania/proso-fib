#include <utils.h>
#include <types.h>
#include <mm.h>
#include <mm_address.h>

void copy_data(void *start, void *dest, int size)
{
  DWord *p = start, *q = dest;
  Byte *p1, *q1;
  while(size > 4) {
    *q++ = *p++;
    size -= 4;
  }
  p1=(Byte*)p;
  q1=(Byte*)q;
  while(size > 0) {
    *q1++ = *p1++;
    size --;
  }
}
/* Copia de espacio de usuario a espacio de kernel, devuelve 0 si ok y -1 si error*/
int copy_from_user(void *start, void *dest, int size)
{
  DWord *p = start, *q = dest;
  Byte *p1, *q1;
  while(size > 4) {
    *q++ = *p++;
    size -= 4;
  }
  p1=(Byte*)p;
  q1=(Byte*)q;
  while(size > 0) {
    *q1++ = *p1++;
    size --;
  }
  return 0;
}
/* Copia de espacio de kernel a espacio de usuario, devuelve 0 si ok y -1 si error*/
int copy_to_user(void *start, void *dest, int size)
{
  DWord *p = start, *q = dest;
  Byte *p1, *q1;
  while(size > 4) {
    *q++ = *p++;
    size -= 4;
  }
  p1=(Byte*)p;
  q1=(Byte*)q;
  while(size > 0) {
    *q1++ = *p1++;
    size --;
  }
  return 0;
}

/* This checks if addr is inside user space */
int check_address(const void *addr,int size){
	unsigned int min_usr_addr, max_usr_addr, max_bytes;

	max_bytes = (NUM_PAG_CODE + NUM_PAG_DATA) * PAGE_SIZE;
	min_usr_addr = L_USER_START;
	max_usr_addr = L_USER_START + max_bytes;

	if ((unsigned int) addr < min_usr_addr
	  || (unsigned int) addr > max_usr_addr || size > max_bytes)
		return 0;

	return 1;

}

/* access_ok: Checks if a user space pointer is valid
 * @type:  Type of access: %VERIFY_READ or %VERIFY_WRITE. Note that
 *         %VERIFY_WRITE is a superset of %VERIFY_READ: if it is safe
 *         to write to a block, it is always safe to read from it
 * @addr:  User space pointer to start of block to check
 * @size:  Size of block to check
 * Returns true (nonzero) if the memory block may be valid,
 *         false (zero) if it is definitely invalid
 */
/* This function checks that the address is accessible in the requested mode (READ or WRITE).
   It controls the page table to do so.
*/
int access_ok(int type, const void *addr, unsigned long size){
	int min_page,max_page;

	if(addr==NULL) return 0;
	if(size<0) return 0;
	
	/* If the first 10bits of the address are greater than ENTRY_DIR_PAGES, error!! */
	if(((int)&addr>>22)>ENTRY_DIR_PAGES) return 0;

	if(check_address(addr,size)==0) return 0;
	
	/* This checks if addr refers to a page which is assigned to this process */
	min_page=((DWord)&addr && 0x003ff000);
	max_page=(min_page+size)>>12;
	min_page=min_page>>12;
	
	//if((DWord)(&addr)<PH_USER_START) return 0;

	if(type==0){ /* Memory read request */
		if((pagusr_table[min_page].bits.present==1)&&
		   (pagusr_table[max_page].bits.present==1)) /* page set --> I think this is the only control I got to perform in "read" case, no?? TO ASK!!! */
			return 1;
	}
	else if(type==1){/* Memory write request */
		if((pagusr_table[min_page].bits.present==1)&&(pagusr_table[max_page].bits.present==1))
			if((pagusr_table[min_page].bits.rw==1)&&(pagusr_table[max_page].bits.rw==1))
				return 1;
		}
	/***/

	return 0;
}


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
int strcmp (const char *s1, const char *s2)
{
  for (; *s1 == *s2; ++s1, ++s2)
    if (*s1 == 0)
      return 0;
  return *(unsigned char *) s1 < *(unsigned char *) s2 ? -1 : 1;
}


