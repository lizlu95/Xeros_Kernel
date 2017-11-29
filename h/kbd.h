/* kbd.h 
 */

#include <xeroskernel.h>

#ifndef KBD_H
#define KBD_H

/* ascii */
#define ENTER 0x0A
#define EOT 0x04
#define BLOCK_PROC -3


extern int kbwrite(void* buff, int bufflen);
extern int kbread(void* buff, int bufflen,void (*cb_func)(void*,int, int),void* p,int count);
extern int kbioctl(void* addr);
extern int kbopen(pcb* p, int fd);
extern int kbclose(pcb* p, int fd);
void kbd_int_handler(void);
extern unsigned char read_char(void);
void save_kchar(unsigned char value);
void save_user_buff(void);
void cleanup (void); 

#endif
