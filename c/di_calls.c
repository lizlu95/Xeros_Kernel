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
    
    //cannot reopen
    if (p->fd_table[fd].status) {
        if (fd == p->fd_table[fd].major) return -1;
    }
    
    p->fd_table[fd].major = devtab[device_no].dvnum;
    p->fd_table[fd].minor = devtab[device_no].dvminor;
    p->fd_table[fd].dev = &devtab[device_no];
    p->fd_table[fd].ctr_data = NULL;
    p->fd_table[fd].status = TRUE;
    
    devsw *devptr = p->fd_table[fd].dev;
    (devptr->dvopen)(p, devptr->dvioblk);
    return fd;
}

extern int di_close(pcb* p, int fd) {
    
    if (fd >= MAX_FD || fd < 0) return -1;
    
    if (p->fd_table[fd].status) {
        if (fd == p->fd_table[fd].major) {
            
            p->fd_table[fd].status = FALSE;
            devsw *devptr = p->fd_table[fd].dev;
            (devptr->dvclose)(p, devptr->dvioblk);
            return 0;
        }
    }
    return -1;
}

extern int di_write(pcb* p, int fd, void* buff, int bufflen) {
    
    if (fd >= MAX_FD || fd < 0) return -1;
    if (!p->fd_table[fd].status) return -1;
    devsw *devptr = p->fd_table[fd].dev;
    return (devptr->dvwrite)(p,devptr->dvioblk,buff, bufflen);
}

extern int di_read(pcb* p, int fd, void* buff, int bufflen, int count) {
    if (fd >= MAX_FD || fd < 0) return -1;
    if (!p->fd_table[fd].status) return -1;
    devsw *devptr = p->fd_table[fd].dev;
    int result = (devptr->dvread)(p,devptr->dvioblk,buff, bufflen);

    return result;
}

extern int di_ioctl(pcb* p, int fd, unsigned long command, void *args) {
    
    if (fd >= MAX_FD || fd < 0) return -1;
    if (!p->fd_table[fd].status) return -1;
    if (p->fd_table[fd].major != KBMON) return -1;
    
    devsw *devptr = p->fd_table[fd].dev;
    (devptr->dvcntl)(p,devptr->dvioblk,command, args);
    return 0;
}


void devinit(void) {
    //unsigned long device_g_buffer[MAX_DEVICE][32];

    for (int i=0; i < MAX_DEVICE; i++) {
        devtab[i].dvnum = KBMON;
        devtab[i].dvopen = (int (*)(pcb *, void *))&kbopen;
        devtab[i].dvclose = (int (*)(pcb *, void *))&kbclose;
        devtab[i].dvinit = (int (*)(void))&kbinit;
        devtab[i].dvwrite = (int (*)(pcb *, void *,void*, int))&kbwrite;
        devtab[i].dvread = (int (*)(pcb *, void *,void*, int))&kbread;
        devtab[i].dvseek = NULL;
        devtab[i].dvgetc = NULL;
        devtab[i].dvputc = NULL;
        devtab[i].dvcntl = (int (*)(pcb *, void *,unsigned long, void*))&kbioctl;
        devtab[i].dvcsr = NULL;
        devtab[i].dvivec = NULL;
        devtab[i].dvovec = NULL;
        devtab[i].dviint = NULL;
        devtab[i].dvoint = NULL;
        devtab[i].dvminor = i;
        // Note: this kmalloc will intentionally never be kfree'd
        devtab[i].dvioblk = (kbd_dvioblk_t*)kmalloc(sizeof(kbd_dvioblk_t));

        ((kbd_dvioblk_t*)devtab[i].dvioblk)->echof = i;
        devtab[i].dvinit();
    }
}