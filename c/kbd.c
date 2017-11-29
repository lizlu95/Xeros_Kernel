/* kbd.c : keyboard device
 */

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>
#include <kbd.h>
#include <stdarg.h>
#include <scancodesToAscii.h>

static unsigned char VALUE; /*used to store value read from keyboard every interrupt */
unsigned int ascii_value;
unsigned int minor;/*minor device number*/
unsigned int major;/*major device number*/
int EOF_flag;/* indicates if EOF key was pressed*/
int kbuff_head = -2;/*circular q head, remove from here*/
int kbuff_tail = -1;/*circular q tail, add from here*/
void (*call_back)(void*,int,int) = NULL;/* function pointer passed in from kbd_read*/
struct pcb* argP = NULL;/* process pointer passed in from kbd_read*/

char* user_buffer = NULL;/* user buffer pointer*/
int user_bufflen = 0; /* bytes the user wants to read*/
int trans_count = 0;/*current position in user buffer,number of bytes read*/
char kkb_buffer[100];
unsigned char EOF_MASK = 0x04; /*used for ioctl 53 */


int kbopen(pcb *p, int fd) {
    enable_irq(1,0);
    major = p->fd_table[fd].major;
    minor = p->fd_table[fd].minor;
    return 0;
}

int kbclose(pcb *p, int fd) {
    enable_irq(1,1);
    cleanup();
    return 0;
}

int kbwrite(void *buff, int bufflen) {
    return -1;
}

int kbread(void *buff, int bufflen, void (*func)(void*,int,int), void* p, int count) {
    trans_count = count;
    
    if (!user_buffer) {
        user_buffer = buff;
        user_bufflen = bufflen;
        call_back = func;
        argP = p;
    }
    
    if (EOF_flag) return 0;
    
    save_user_buff();
    
    if(trans_count == user_bufflen) {
        trans_count = 0;
        return user_bufflen;
    } else if (trans_count < user_bufflen)
        return BLOCK_PROC;
    return -1;
}

int kbioctl(void* addr) {
    va_list ap = (va_list)addr;
    int command = va_arg(ap, int);
    unsigned long *outc;
    
    int result = 0;
    
    switch(command) {
        case SET_EOF :
            outc = (unsigned long *)va_arg(ap, int);
            
            char temp = *outc;
            *outc = EOF_MASK;
            EOF_MASK = temp;
            break;
        case ECHO_OFF :
            minor = 0;
            break;
        case ECHO_ON :
            minor = 1;
            break;
        default :
            result = -1;
            break;
    }
    return result;
}


void kbd_int_handler(void) {
    
    /* read from keyboard input port*/
    unsigned char in_value = read_char();
    /* if no EOF key detected*/
    if(!EOF_flag){
        
        /* if it's not key up*/
        if(in_value != NOCHAR && in_value != 0){
            
            /*if is echo version, print*/
            if(minor == 1){
                kprintf("%c",in_value);
            }
            
            /* if there is process wating*/
            if(user_buffer != NULL){
                /* write character to user buffer*/
                user_buffer[trans_count] = (char)in_value;
                // increment counter
                trans_count++;
                
                /*check if read is complete*/
                if(trans_count == user_bufflen){
                    /* reset counter and user buffer*/
                    trans_count = 0;
                    user_buffer = NULL;
                    /*unblock process*/
                    call_back(argP,user_bufflen, trans_count);
                    
                }
                /* if ENTER detected */
                if(in_value == ENTER){
                    /* saves the current count number*/
                    int tmp = trans_count;
                    /* reset counter and user buffer */
                    trans_count = 0;
                    user_buffer = NULL;
                    /* unblock process */
                    call_back(argP,tmp, trans_count);
                }
            }
            else{
                /*block queue is empty,save to kernel buffer*/
                save_kchar(in_value);
            }
        }
    }
    else{/* EOF key is detected, return with 0*/
        if(call_back != NULL){
            call_back(argP,0, trans_count);
        }
    }
}

extern unsigned char read_char(void) {
    __asm __volatile( " \
                     pusha  \n\
                     in  $0x60, %%al  \n\
                     movb %%al, VALUE  \n\
                     popa  \n\
                     "
                     :
                     :
                     : "%eax"
                     );
    /* change the scancode back to ASCII */
    ascii_value = kbtoa(VALUE);
    if(ascii_value == EOT || ascii_value == EOF_MASK){
        /*set EOF flag*/
        EOF_flag = TRUE;
        
    }
    
    return ascii_value;
}

void save_kchar(unsigned char value){
    
    /*if kernel buff is empty*/
    if(kbuff_head == -2 && kbuff_tail == -1){
        kbuff_head = 0;
        kbuff_tail = 0;
        /*write value in*/
        kkb_buffer[kbuff_tail]=value;
        /* increment tail pointer*/
        kbuff_tail++;
    }
    /*if kernel buffer is not full yet,add value*/
    else if(kbuff_head != kbuff_tail){
        kkb_buffer[kbuff_tail] = value;
        /* increment tail index*/
        kbuff_tail++;
        kbuff_tail = (kbuff_tail) % KKB_BUFFER_SIZE;
    }
    
}

void save_user_buff(void) {
    /*get how much more the process want to read*/
    int left_to_read = user_bufflen - trans_count;
    int i;
    /* for each desire byte to read*/
    for (i = 0; i < left_to_read; i++){
        /* if kernel buffer is not empty*/
        if(kbuff_head != -2 && kbuff_tail != -1){
            /* copy the value from kernel buffer to user buffer*/
            user_buffer[trans_count] = kkb_buffer[kbuff_head];
            /* increment user buffer counter*/
            trans_count++;
            /* increment kernel head index */
            kbuff_head ++;
            kbuff_head = (kbuff_head)%KKB_BUFFER_SIZE;
        }
        /* if kernel buffer is empty , reset head and tail index*/
        if(kbuff_head == kbuff_tail){
            kbuff_head = -2;
            kbuff_tail = -1;
        }
    }
}


void cleanup(void) {
    
    minor = -1;
    major = -1;
    EOF_flag = FALSE;
    kbuff_head = -2;
    kbuff_tail = -1;
    
    call_back = NULL;
    argP = NULL;
    
    user_buffer = NULL;
    user_bufflen = 0;
    trans_count = 0;
}

