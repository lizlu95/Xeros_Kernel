/* kbd.c: Keyboard device
*/

#include <stdarg.h>
#include <xeroslib.h>
#include <kbd.h>
#include <i386.h>
#include <scancodesToAscii.h>


int kbinit(void) {
    int i;
    
    count = 0;
    done = 0;
    buffer_head = 0;
    buffer_tail = 0;
    taskcnt = 0;
    
    for (i = 0; i < PROCARR; i++) {
        procarr[i].waiting = 0;
    }
    
    inb(PORT_DATA);
    inb(PORT_CTRL);
    return 0;
}

int kbopen(pcb *proc, void *dvioblk) {
    int echo_flag = ((kbd_dvioblk_t*)dvioblk)->echof;
    
    if (count > 0) {
        if (type != echo_flag) return -1;
        
        count++;
        return 0;
    }
    
    count = 1;
    type = echo_flag;
    done = 0;
    buffer_head = 0;
    buffer_tail = 0;
    keysf = 0;
    eof = DEFEOF;
    echof = echo_flag;
    
    enable_irq(1,0);
    return 0;
}

int kbclose(pcb *proc, void *dvioblk) {
    if (count < 0) return -1;

    count--;
    if (count == 0) enable_irq(1,1);
    
    if (procarr[proc->pid % PROCARR].waiting) {
        taskcnt--;
    }
    
    procarr[proc->pid % PROCARR].waiting = 0;
    return 0;
}

int kbread(pcb *proc, void *dvioblk, void* buf, int buflen) {
    
    kbdproc_t *task = &procarr[proc->pid % PROCARR];
    taskcnt++;
    
    task->pcb = proc;
    task->buf = buf;
    task->i = 0;
    task->buflen = buflen;
    task->waiting = 1;
    get_buf();
    if (task->i == buflen) {
        return buflen;
    }
    
    if (done) return task->i;
    return BLOCKERR;
}

int kbwrite(pcb *proc, void *dvioblk, void* buf, int buflen) {
    return -1;
}

int kbioctl(pcb *proc, void *dvioblk, unsigned long command, void *args) {
    
    switch(command) {
        case SET_EOF:
            return set_eof(args);
        case ECHO_ON:
            echof = 1;
            return 0;
        case ECHO_OFF:
            echof = 0;
            return 0;
        default:
            return SYSERR;
    }
}


static int set_eof(void *args) {
    va_list ap;
    
    if (args == NULL) {
        return SYSERR;
    }
    
    ap = (va_list)args;
    eof = (char)va_arg(ap, int);
    
    return 0;
}

void kbd_int_handler(void) {
    int hasData;
    int data;
    char c = 0;
    
    hasData = inb(PORT_CTRL) & 0x01;
    data = inb(PORT_DATA) & 0xff;
    
    if (hasData) {
        c = kbtoa(data);
        
        if (c != 0) {
            if (echof && c != eof) kprintf("%c", c);
            
            if (taskcnt > 0) get_char(c);
            else if (((buffer_head + 1) % KBBUFSIZE) != buffer_tail) {
                kbbuffer[buffer_head] = c;
                buffer_head = (buffer_head + 1) % KBBUFSIZE;
            }
        }
    }
}

static void get_buf(void) {
    char c;
    
    while (taskcnt > 0 &&
        buffer_tail != buffer_head) {
            
        c = kbbuffer[buffer_tail];
        get_char(c);
        buffer_tail = (buffer_tail + 1) % KBBUFSIZE;
    }
}


static void get_char(char c) {
    int i;
    kbdproc_t *task;
    
    if (c == eof) {
        handleeof();
        return;
    }
    
    for (i = 0; i < PROCARR; i++) {
        if (procarr[i].waiting) {
            task = &procarr[i];
            ((char*)(task->buf))[task->i] = c;
            task->i++;
            
            if (task->i == task->buflen || c == '\n') {
                unblock(task);
            }
        }
    }
}


static void handleeof(void) {
    int i;
    kbdproc_t *task;
    
    enable_irq(1,1);
    done = 1;

    for (i = 0; i < PROCARR; i++) {
        if (procarr[i].waiting) {
            task = &procarr[i];
            unblock(task);
        }
    }
    
    return;
}

static void unblock(kbdproc_t *task) {
    taskcnt--;
    task->waiting = 0;
    task->pcb->ret = task->i;
    if (task->pcb->state == STATE_BLOCK) {
        ready(task->pcb);
    }
}
