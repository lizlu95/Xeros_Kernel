/* signal.c - support for signal handling
   This file is not used until Assignment 3
 */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <i386.h>

/* Your code goes here */
int sighandler(pcb* p, int signal, void (*newhandler) (void*), void (**oldHandler) (void*)){

	if (signal < 0 || signal >= 32) return -1;
	if ((unsigned long)*oldHandler == &sysstop) return -1;
	if (((unsigned long) newhandler) >= HOLESTART 
		&& ((unsigned long) newhandler <= HOLEEND))
		return -2;

	*oldHandler = p->sig_table[signal];
	p->sig_table[signal] = newhandler;
			
	return 0;
}

int wait(int pid, pcb *p) {

	pcb* waitp = findPCB(pid);
	if(pid == 0 || waitp == NULL) return -1;
	else{	
		removeFromReady(p);
		waitp->blockQ = enQ(waitp->blockQ, p);
		p->state = STATE_BLOCK;
		p->waitfor = waitp;
		return 0;
	}
}


void sigtramp(void (*handler)(void*), void *cntx) {
	handler(cntx);
	syssigreturn(cntx);
}


int signal(int pid, int signal) {
	pcb *proc = findPCB(pid);
	proc->sigmark[signal] = 1;

	if (proc->state == STATE_BLOCK) {
		proc->ret = -99;
		proc->waitfor->blockQ = remove(proc->waitfor->blockQ, proc);
		proc->waitfor = NULL;
		ready(proc);
	}
	if (proc->state == STATE_SLEEP) {
		proc->ret = currtick(proc);
		removeFromSleep(proc);
		ready(proc);
	}

	return 0;
}