#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>

#ifdef _DEBUG
#define PRINTF printf
#define FFLUSH fflush(stdout);
#define GETCHAR getchar
#else
#define PRINTF
#define FFLUSH
#define GETCHAR
#endif

#define BUFCAPACITY 512
u_char    g_byte             = 0;
u_char    g_bitpos           = 0;
u_char    g_buf[BUFCAPACITY] = {0};
int       g_bufsize          = 0;

void SIGCHLD_handler(int);
void bit_handler(int);      // SIGUSR1 = 0, SIGUSR2 = 1
void empty_handler(int);
void childSIGTSTP_handler(int);
void parentSIGTSTP_handler(int);
void parent(pid_t);
void child(char*);

#define ASSERT(obj, expr) if(!(expr)) {\
                            perror(#obj);\
                            exit(EXIT_FAILURE);}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        errno = EINVAL;
        perror("");
        exit(EXIT_FAILURE);
    }

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGTSTP);
    ASSERT(sigprocmask,sigprocmask(SIG_BLOCK, &set, NULL) >= 0)

// setting handlers
    struct sigaction sa = {0};
    sa.sa_handler = SIGCHLD_handler;
    sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT;
    ASSERT(sigaction, sigaction(SIGCHLD, &sa, NULL) >= 0)

    sa.sa_handler = bit_handler;
    sa.sa_flags = 0;
    ASSERT(sigaction, sigaction(SIGUSR1, &sa, 0) >= 0)
    ASSERT(sigaction, sigaction(SIGUSR2, &sa, 0) >= 0)

    sa.sa_handler = empty_handler;
    ASSERT(sigaction, sigaction(SIGALRM, &sa, NULL) >= 0)

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGCHLD);
    sa.sa_handler = parentSIGTSTP_handler;
    ASSERT(sigaction, sigaction(SIGTSTP, &sa, 0) >= 0)  //setting SIGTSTP handler SIGCHLD blocking
//
    pid_t pid = fork();

    switch(pid) {
        case 0:
            child(argv[1]);
            break;

        case -1:
            perror("fork");
            exit(EXIT_FAILURE);

        default:
            parent(pid);
    }
    return 0;
}

void flush(int nbytes) {
    write(STDOUT_FILENO, g_buf, nbytes);
    g_bufsize = 0;
}

//=============HANDLERS=============//
char g_catched = 0;   // for child:
                      // 0 - parent isn't ready to take bit as signal, 1 - parent is ready
                      // for parent:
                      // 1 - received SIGUSR1 or SIGUSR2, 2 - recieved SIGTSTP from child, 0 - received another signal

void SIGCHLD_handler(int signal) {
    fprintf(stderr, "child dead");
    fflush(stderr);
    exit(EXIT_FAILURE);
}

void bit_handler(int signal) {
    if(signal == SIGUSR1)
        g_byte &= ~(0x80U >> g_bitpos);

    if(signal == SIGUSR2)
        g_byte |=  (0x80U >> g_bitpos);

    g_bitpos++;
    g_catched = 1;
}

void empty_handler(int signal) {}

void childSIGTSTP_handler(int signal) {
    g_catched = 1;
}

void parentSIGTSTP_handler(int signal) {
    struct sigaction sa = {0};
    sa.sa_handler = SIG_DFL;
    ASSERT(sigaction, sigaction(SIGCHLD, &sa, NULL) >= 0)
    flush(g_bufsize);
    g_catched = 2;
}
//==================================//

void parent(pid_t cpid) {
    sigset_t empty_set;
    sigemptyset(&empty_set);

    while(1) {
        while((g_catched != 1) && (g_catched != 2)) {
            alarm(1U);
            sigsuspend(&empty_set);
            alarm(0);
        }

        switch(g_catched) {
            case 1:
                if(g_bitpos == 8) {
                    g_buf[g_bufsize] = g_byte;
                    ++g_bufsize;
                    if(g_bufsize == BUFCAPACITY)
                        flush(BUFCAPACITY);
                    g_bitpos = 0;
                }
                kill(cpid, SIGTSTP);
                g_catched = 0;
                break;
            case 2:
                g_catched = 0;
                goto out;
            default:
                fprintf(stderr, "smth went wrong...");
                fflush(stderr);
                exit(EXIT_FAILURE);
        }
    }
    out:
        wait(NULL);
    exit(EXIT_SUCCESS);
}

void child(char* datapath) { // child process
    struct sigaction sa = {0};
    sa.sa_handler = childSIGTSTP_handler;
    ASSERT(sigaction, sigaction(SIGTSTP, &sa, NULL) >= 0)

    sigset_t empty_set;
    sigemptyset(&empty_set);

    int fd = open(datapath, O_RDONLY);
    ASSERT(datapath_open, fd > 0)

    int readed = 0;
    pid_t ppid = getppid();
    g_catched   = 0;

    do {
        readed = read(fd, g_buf, BUFCAPACITY);
        ASSERT(data_read, readed >= 0)

        for(int byte = 0; byte < readed; byte++) {
            for(int bit = 7; bit >= 0; bit--) {
                if((g_buf[byte] >> bit) & 1U)
                    kill(ppid, SIGUSR2);
                else
                    kill(ppid, SIGUSR1);

                while(!g_catched) {
                    alarm(1U);
                    sigsuspend(&empty_set);
                    alarm(0);

                    if(ppid != getppid()) {
                        fprintf(stderr, "parent dead");
                        fflush(stderr);
                        exit(EXIT_FAILURE);
                    }
                } g_catched = 0;
            }
        }

    } while(readed == BUFCAPACITY);

    kill(ppid, SIGTSTP);
    close(fd);

    exit(EXIT_SUCCESS);
}