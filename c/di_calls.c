/* di_calls.c: device calls
 */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>
#include <tty.h>
#include <kbd.h>

pcb *blockQ;
devsw devtab[MAX_DEVICE];

extern int di_open(pcb* p, int device_no) {
    
    if (device_no >= MAX_DEVICE || device_no < 0) return -1;
    int fd = devtab[device_no].dvnum;
    
    if (p->fd_table[fd].status) {
        if (fd == p->fd_table[fd].major) {
            // cant reopen
            if (p->fd_table[fd].minor == devtab[device_no].dvminor) return -1;
            // cant open same major
            else return -1;
        }
    }
    
    p->fd_table[fd].major = devtab[device_no].dvnum;
    p->fd_table[fd].minor = devtab[device_no].dvminor;
    p->fd_table[fd].dev = &devtab[device_no];
    p->fd_table[fd].ctr_data = NULL;
    p->fd_table[fd].status = TRUE;
    
    devsw *devptr = p->fd_table[fd].dev;
    (devptr->dvopen)(p, fd);
    return fd;
}

extern int di_close(pcb* p, int fd) {
    
    if (fd >= MAX_FD || fd < 0) return -1;
    
    if (p->fd_table[fd].status) {
        if (fd == p->fd_table[fd].major) {
            
            p->fd_table[fd].status = FALSE;
            devsw *devptr = p->fd_table[fd].dev;
            (devptr->dvclose)(p, fd);
            return 0;
        }
    }
    return -1;
}

extern int di_write(pcb* p, int fd, void* buff, int bufflen) {
    
    if (fd >= MAX_FD || fd < 0) return -1;
    if (!p->fd_table[fd].status) return -1;
    devsw *devptr = p->fd_table[fd].dev;
    return (devptr->dvwrite)(buff, bufflen);
}

extern int di_read(pcb* p, int fd, void* buff, int bufflen, int count) {
    
    if (fd >= MAX_FD || fd < 0) return -1;
    if (!p->fd_table[fd].status) return -1;
    
    if (blockQ) {
        p->fd = fd;
        p->buffer = buff;
        p->bufferlen = bufflen;
        blockQ = enQ(blockQ, p);
        p->state = STATE_BLOCK;
        removeFromReady(p);
        return -2;
    }
    
    devsw *devptr = p->fd_table[fd].dev;
    int result = (devptr->dvread)(buff, bufflen, (void (*)(void*, int,int))&unblock_read, (void *) p, count);
    
    if (result == 999) {
        p->fd = fd;
        p->buffer = buff;
        p->bufferlen = bufflen;
        removeFromReady(p);
        p->next = blockQ;
        blockQ = p;
        p->state = STATE_BLOCK;
        return -2;
    }
    return result;
}

extern int di_ioctl(pcb* p, int fd, void *addr) {
    
    if (fd >= MAX_FD || fd < 0) return -1;
    if (!p->fd_table[fd].status) return -1;
    if (p->fd_table[fd].major != 999) return -1;
    
    devsw *devptr = p->fd_table[fd].dev;
    (devptr->dvcntl)((void *)addr);
    return 0;
}

extern void unblock_read(pcb* p, int len, int count) {
    if (!blockQ) return;
    if (blockQ == p) {
        blockQ = blockQ->next;
        p->next = NULL;
        p->ret = len;
        ready(p);
        
        if (blockQ)
            di_read(blockQ, blockQ->fd,
                    blockQ->buffer, blockQ->bufferlen, count);
    }
    return;
}

void devinit(void) {
    unsigned long device_g_buffer[MAX_DEVICE][32];
    for (int i=0; i < MAX_DEVICE; i++) {
        devtab[i].dvnum = KBMON;
        devtab[i].dvopen = (int (*)(void*, int))&kbopen;
        devtab[i].dvclose = (int (*)(void*, int))&kbclose;
        devtab[i].dvinit = NULL;
        devtab[i].dvwrite = (int (*)(void*, int))&kbwrite;
        devtab[i].dvread = (int (*)(void*, int, void (*)(void*,int, int),void*, int))&kbread;
        devtab[i].dvseek = NULL;
        devtab[i].dvgetc = NULL;
        devtab[i].dvputc = NULL;
        devtab[i].dvcntl = (int (*)(void*))&kbioctl;
        devtab[i].dvcsr = NULL;
        devtab[i].dvivec = NULL;
        devtab[i].dvovec = NULL;
        devtab[i].dviint = NULL;
        devtab[i].dvoint = NULL;
        devtab[i].dvioblk = (void *)device_g_buffer+i*32;   
        devtab[i].dvminor = i;
    }
    
}