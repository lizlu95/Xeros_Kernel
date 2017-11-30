/* kbd.h
 */

#include <xeroskernel.h>

#define DEFEOF ((char)0x04)
#define PROCARR (MAX_PROC)
#define KBBUFSIZE (4 + 1)

typedef struct kbdproc {
    pcb *pcb;
    void *buf;
    int buflen;
    int i;
    int waiting;
} kbdproc_t;

typedef struct kbd_dvioblk {
    int echof;
} kbd_dvioblk_t;

static int keysf = 0;
static char kbbuffer[KBBUFSIZE] = {0};
static int buffer_head = 0;
static int buffer_tail = 0;
static char eof;
static char echof;
static kbdproc_t procarr[PROCARR];
static int taskcnt = 0;
static int count = 0;
static int type = 0;
static int done = 0;

static int set_eof(void *args);
static void get_buf(void);
static void unblock(kbdproc_t *task);
static void get_char(char c);
static void handleeof(void);


int kbinit(void);
int kbopen(pcb *proc, void *dvioblk);
int kbclose(pcb *proc, void *dvioblk);
int kbread(pcb *proc, void *dvioblk, void* buf, int buflen);
int kbwrite(pcb *proc, void *dvioblk, void* buf, int buflen);
int kbioctl(pcb *proc, void *dvioblk, unsigned long command, void *args);
void kbd_int_handler(void);