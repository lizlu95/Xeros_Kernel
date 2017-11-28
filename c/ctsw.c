/* ctsw.c : context switcher
 */

#include <xeroskernel.h>
#include <i386.h>


void _KernelEntryPoint(void);
void _TimerEntryPoint(void);
void _KeyboardEntryPoint(void);

static void               *saveESP;
static unsigned int        rc;
static int                 trapNo;
static long                args;

int contextswitch( pcb *p ) {
/**********************************/

    /* keep every thing on the stack to simplfiy gcc's gesticulations
     */
    int i = MAX_SIG;
    int sig = -1;
    while (i-->0)
        if (p->sigmark[i]) {
            sig = i;
            break;
        } 

    if (sig) {
        actualsignal(p, sig);
        p->sigmark[sig] = 0;
    }

    saveESP = p->esp;
    rc = p->ret; 
 
    /* In the assembly code, switching to process
     * 1.  Push eflags and general registers on the stack
     * 2.  Load process's return value into eax
     * 3.  load processes ESP into edx, and save kernel's ESP in saveESP
     * 4.  Set up the process stack pointer
     * 5.  store the return value on the stack where the processes general
     *     registers, including eax has been stored.  We place the return
     *     value right in eax so when the stack is popped, eax will contain
     *     the return value
     * 6.  pop general registers from the stack
     * 7.  Do an iret to switch to process
     *
     * Switching to kernel
     * 1.  Push regs on stack, set ecx to 1 if timer interrupt, jump to common
     *     point.
     * 2.  Store request code in ebx
     * 3.  exchange the process esp and kernel esp using saveESP and eax
     *     saveESP will contain the process's esp
     * 4a. Store the request code on stack where kernel's eax is stored
     * 4b. Store the timer interrupt flag on stack where kernel's eax is stored
     * 4c. Store the the arguments on stack where kernel's edx is stored
     * 5.  Pop kernel's general registers and eflags
     * 6.  store the request code, trap flag and args into variables
     * 7.  return to system servicing code
     */
 
    __asm __volatile( " \
        pushf \n\
        pusha \n\
        movl    rc, %%eax    \n\
        movl    saveESP, %%edx    \n\
        movl    %%esp, saveESP    \n\
        movl    %%edx, %%esp \n\
        movl    %%eax, 28(%%esp) \n\
        popa \n\
        iret \n\
   _KeyboardEntryPoint:  \n\
        cli  \n\
        pusha  \n\
        movl $2, %%ecx  \n\
        jmp _CommonJumpPoint  \n\
   _TimerEntryPoint: \n\
        cli   \n\
        pusha \n\
        movl    $1, %%ecx \n\
        jmp     _CommonJumpPoint \n \
   _KernelEntryPoint: \n\
        cli \n\
        pusha  \n\
        movl   $0, %%ecx \n\
   _CommonJumpPoint: \n \
        movl    %%eax, %%ebx \n\
        movl    saveESP, %%eax  \n\
        movl    %%esp, saveESP  \n\
        movl    %%eax, %%esp  \n\
        movl    %%ebx, 28(%%esp) \n\
        movl    %%ecx, 24(%%esp)\n		\
        movl    %%edx, 20(%%esp) \n\
        popa \n\
        popf \n\
        movl    %%eax, rc \n\
        movl    %%ecx, trapNo \n\
        movl    %%edx, args \n\
        "
        : 
        : 
        : "%eax", "%ebx", "%edx"
    );

    /* save esp and read in the arguments
     */
    p->esp = saveESP;
    if( trapNo == 1 ) {
    	/* return value (eax) must be restored, (treat it as return value) */
    	p->ret = rc;
    	rc = SYS_TIMER;
    } else if (trapNo == 2) {
        p->ret = rc;
        rc = SYS_KB;
    } else {
        p->args = args;
    }
    return rc;
}

void contextinit( void ) {
/*******************************/
  kprintf("Context init called\n");
  set_evec( KERNEL_INT, (int) _KernelEntryPoint );
  set_evec( TIMER_INT,  (int) _TimerEntryPoint );
  set_evec( KB_INT,  (int) _KeyboardEntryPoint );
  initPIT( 100 );

}


void actualsignal(pcb *p, int sig) {
    
    unsigned long * ESP = p->esp;
    *ESP = (unsigned long) p->ret;
    ESP--; 
    *ESP = (unsigned long) p->esp;      
    ESP--;
    *ESP = (unsigned long) p->sig_table[sig];                     
    ESP--;
    *ESP = 0;
    p->esp = ESP;

    /* need a fake context */
    struct context_frame *ctfm = (struct context_frame *)(p->esp - sizeof(struct context_frame)/sizeof(unsigned long));
    ctfm->edi = 0;
    ctfm->esi = 0;
    ctfm->ebp = 0;
    ctfm->esp = 0;
    ctfm->ebx = 0;
    ctfm->edx = 0;
    ctfm->ecx = 0;
    ctfm->eax = 0;
    ctfm->iret_eip = (unsigned long) &sigtramp;
    ctfm->iret_cs = getCS();
    ctfm->eflags = 0x00003200;
    /* update esp value */
    p->esp = (unsigned long *) ctfm;
}
