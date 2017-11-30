/* user.c : User processes
 */

#include <xeroskernel.h>
#include <xeroslib.h>

int kill, tickn, shell_pid;

extern void root(void) {
    char *username = "cs415";
    char *password = "EveryonegetsanA";
    
    for(;;) {
        char user[10];
        char pw[40];
        
        sysputs("\nWelcome to Xeros - an experimental OS\n");
        
        int fd = sysopen(0);
        //sysioctl(fd, ECHO_ON);
        sysputs("Username: ");
        sysread(fd, user, 10);
        sysioctl(fd, ECHO_OFF);
        
        sysputs("\nPassword: ");
        sysread(fd, pw, 40);
        sysclose(fd);
        
        if (strncmp(user, username, 5) == 0 &&
            strncmp(pw, password, 15) == 0) {
            sysputs("Success!\n");
            shell_pid = syscreate(&shell, PROC_STACK);
            syswait(shell_pid);
        }
    }
}


void shell(void) {
    char buffer[100];
    int fd = sysopen(1);
    
    for(;;) {
        sysputs("\n> ");
        
        int b = sysread(fd, buffer, 100);
        if (!b) break;
        
        char command[50];
        char arg[50];
        checkcmd(buffer,100, command, 50);

        int ap;
        for (ap=0; ap<50; ap++)
            if (buffer[ap] == '&') break;
        ap = (ap == b-2) ? 1:0;

        int wait = 1;
        int pid;
        
        if(!strcmp("ps", command)) {
            processStatuses ps;
            char str[80];
            int t = sysgetcputimes(&ps);
            sysputs("\nPID | State     | Time \n");
            for (int i = 0; i <= t; i++) {
                int s = ps.status[i];
                char *state;
                switch(s) {
                    case STATE_STOPPED: state = "STOPPED"; break;
                    case STATE_READY: state = "READY"; break;
                    case STATE_SLEEP: state = "SLEEPING"; break;
                    case STATE_RUNNING: state = "RUNNING"; break;
                    case STATE_BLOCK: state = "BLOCKED"; break;
                    default: break;
                }
                sprintf(str, "%4d  %10s  %6d\n", ps.pid[i],
                        state, ps.cpuTime[i]);
                sysputs(str);
            }
            continue;
        }
        else if(!strcmp("ex", command)) break;
        else if(!strcmp("k", command)) {
            kill = atoi(arg);
            if (!findPCB(kill)) sysputs("No such process\n");
            else syskill(kill, 31);
            continue;
        }
        else if(!strcmp("a", command)) {
            tickn = atoi(arg);
            pid = syscreate(&a, PROC_STACK);
            wait = !ap;
        }
        else if(!strcmp("t", command) && !ap)
            pid = syscreate(&t, PROC_STACK);
        else {
            sysputs("Command not found\n");
            wait = 0;
            continue;
        }
        
        if (wait) syswait(pid);
    }
}

void checkcmd(char * buffer, int bufflen, char *command, int command_len){
    int i,j;
    for (i=0; i<bufflen; i++)
        if (buffer[i] == ' ') break;
    for (j=0; i<bufflen; i++)
        if (buffer[i] == '\n') break;

    if (i <= command_len-1 || j <= command_len-1) {
        if (i < j) strncpy(command, buffer, i);
        else strncpy(command, buffer, j);  

        if (command[i-1] == '\n') {
            kprintf("i'm here");
            command[i-1] = '\0';
        }
        command[i+1] = '\0';        
    }
}


void ahandler(void) {
    funcptr1 old;
    sysputs("ALARM ALARM ALARM\n");
    syssighandler(15, NULL, &old);
}

void a(void) {
    funcptr1 old;
    syssleep(tickn*10);
    syssighandler(15, (funcptr1)&ahandler, &old);
    syskill(shell_pid, 15);
}

void t(void) {
    for(;;) {
        sysputs("T\n");
        syssleep(10000);
    }
}

