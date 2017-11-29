/* user.c : User processes
 */

#include <xeroskernel.h>
#include <xeroslib.h>

int kill, tickn, shell_pid;

extern void root(void) {
    static char *username = "cs415";
    static char *password = "EveryonegetsanA";
    
    for(;;) {
        char user[10];
        char pw[40];
        
        sysputs("\nWelcome to Xeros - an experimental OS\n");
        
        int fd = sysopen(0);
        
        sysputs("Username: \n");
        sysread(fd, user, 10);
        
        sysioctl(fd, ECHO_OFF);
        
        sysputs("Password: \n");
        sysread(fd, pw, 40);
        
        sysclose(fd);
        
        if (strcmp(user, username) == 0 &&
            strcmp(pw, password) == 0) {
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
        int ap = endinand(buffer, command, arg);
        
        int wait = 1;
        int pid;
        
        if(!strcmp("ps", command)) {
            processStatuses ps;
            char str[80];
            int t = sysgetcputimes(&ps);
            sysputs("\nPID | State     | Time \n");
            for (int i = 0; i <= t; i++) {
                sprintf(str, "%4d  %10s  %8d\n", ps.pid[i],
                        ps.status[i], ps.cpuTime[i]);
                sysputs(str);
            }
        }
        else if(!strcmp("ex", command)) break;
        else if(!strcmp("k", command)) {
            kill = atoi(arg);
            if (!findPCB(kill)) sysputs("No such process\n");
            else syskill(kill, 31);
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
        }
        
        if (wait) syswait(pid);
    }
}


int endinand(char *buffer, char *command, char *arg) {
    int ampersand = 0;
    int len = 0;
    
    while (*buffer != '\0' && *buffer != ' ' && *buffer != '&') {
        *command = *buffer;
        command++;
        buffer++;
        len++;
    }
    
    *command = '\0';
    
    // get to first argument
    while ((*buffer != '\0') && (*buffer == ' ' || *buffer == '&')) {
        buffer++;
        len++;
    }
    
    // read argument
    while (*buffer != '\0' && *buffer != ' ' && *buffer != '&') {
        *arg = *buffer;
        arg++;
        buffer++;
        len++;
    }
    
    *arg = '\0';
    
    // go to the end of the command line, then we'll read to see if we hit a &
    while(*buffer != '\0') {
        buffer++;
        len++;
    }
    
    len--;
    buffer--;
    while((len > 0) && (*buffer == ' ' || *buffer == '&')) {
        if (*buffer == '&') {
            ampersand = 1;
        }
        if (*buffer != ' ') {
            break;
        }
        buffer--;
        len--;
    }
    
    return ampersand;
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

