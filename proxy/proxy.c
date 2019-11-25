#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <math.h>
#include <sys/select.h>

#ifdef _DEBUG
#define PRINTF printf
#define FFLUSH fflush(stdout);
#define KILL_CHILD {printf("KILL CHILD!!"); getchar();}
#else
#define PRINTF(...)
#define FFLUSH
#define KILL_CHILD
#endif

#define ASSERT(expr, code) do {\
                              if(!(code)){\
                                perror(#expr);\
                                exit(EXIT_FAILURE); }}\
                           while(0)

#define CLDBUFSIZE  4096
#define KBYTE       1024
#define MAXCAPACITY 128
#define MAXPOWER    4

#define MAX(a,b) ((a) > (b)) ? (a) : (b)

typedef struct connection {
    int     fds[2];
    char*   buf;
    int     buf_capacity;
    int     size;
    int     offset;
    int     dead;       // 0 - alive, 1 - achieved eof, we are dying
} connection_t;

long get_number(const char* str) {
    char* str_end;
    long number = strtol(str, &str_end, 10);

    if ((str_end == str) || (errno != 0 && number == 0))
        return -1;

    return number;
}

int     create_connections(int, connection_t*, int*, int);
void    child(int*);
void    server(connection_t*, int);

#ifdef _TEST
int test_out = 0;
#define STDOUT_FILENO test_out
#endif

int main(int argc, char** argv) {

    if(argc != 3) {
        fprintf(stderr, "%s <file path> <child number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int child_num = get_number(argv[2]);
    if(child_num < 0) {
        errno = EINVAL;
        perror("");
        exit(EXIT_FAILURE);
    }

    int src_fd = open(argv[1], O_RDONLY);
    if(src_fd < 0) {
        fprintf(stderr, "can't open %s", argv[1]);
        exit(EXIT_FAILURE);
    }

#ifdef _TEST
test_out = open("output.txt", O_WRONLY | O_TRUNC);
ASSERT(test_out, test_out >= 0);
#endif

    //children won't become a zombie
    struct sigaction act = {.sa_handler = SIG_DFL, .sa_mask = SA_NOCLDWAIT};
    ASSERT(sigaction, sigaction(SIGCHLD, &act, NULL) >= 0);

    connection_t* con_buf = (connection_t*) calloc(child_num, sizeof(connection_t));
    assert(con_buf);

    int child_fds[2] = {};
    switch(create_connections(child_num, con_buf, child_fds, src_fd)) {
        case 1:
            server(con_buf, child_num);
            close(src_fd);

            for(int i  = 0; i < child_num; i++)
                free(con_buf[i].buf);

            break;
        case 0:
            child(child_fds);
    }
    free(con_buf);

    return 0;
}

void allocate_connectionbufs(int con_sz, connection_t* con_buf) {
    for(int i = 0; i < con_sz; i++) {
        con_buf[i].buf_capacity = ((con_sz - i > MAXPOWER) ? MAXCAPACITY :
                                            (int) pow(3,(double) (con_sz - i))) * KBYTE;
        con_buf[i].size         = 0;
        con_buf[i].offset       = 0;
        con_buf[i].dead         = 0;
        con_buf[i].buf          = (char*) calloc(con_buf[i].buf_capacity, sizeof(char));
        assert(con_buf[i].buf);
    }
}

/**
 * @param child_num
 * @param con_buf
 * @param child_fds
 * @return
 *         parent: 1
 *         child:  0
 */
int create_connections(int con_sz, connection_t* con_buf, int* child_fds, int FirstChildRdFd) {
    assert(con_buf && child_fds && con_sz >= 0 && FirstChildRdFd >= 0);
    int fds1[2]  = {};
    int fds2[2]  = {};
    
    ASSERT(pipe, pipe(fds1) >= 0);

#ifdef _DEBUG
    pid_t pid = 0;
#define PID pid=
#else
#define PID
#endif

    switch( PID fork()) { // first child
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);

        case 0:
            close(fds1[0]);
            child_fds[0] = FirstChildRdFd;
            child_fds[1] = fds1[1];
            return 0;

        default:
            PRINTF("%d ", pid);
            FFLUSH
            close(fds1[1]);
            con_buf[0].fds[0] = fds1[0];
    }

    for(int i = 0; i < con_sz - 1; i++) {
        ASSERT(pipe, pipe(fds1) >= 0);
        ASSERT(pipe, pipe(fds2) >= 0);

        switch(PID fork()) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
                
            case 0:
                for(int j = 0; j < i; j++) {    // close all inherited descriptors
                    close(con_buf[j].fds[0]);
                    close(con_buf[j].fds[1]);
                }
                close(fds1[1]); // close write end in child first pipe
                close(fds2[0]); // close read end in child second pipe
                child_fds[0] = fds1[0];
                child_fds[1] = fds2[1];
                return 0;

            default:
                PRINTF("%d ", pid);
                close(fds1[0]);
                close(fds2[1]);
                fcntl(fds1[1], F_SETFL, O_WRONLY | O_NONBLOCK);
                fcntl(fds2[0], F_SETFL, O_RDONLY | O_NONBLOCK);
                con_buf[i].fds[1]   = fds1[1];
                con_buf[i+1].fds[0] = fds2[0];
        }
    }

    con_buf[con_sz - 1].fds[1] = STDOUT_FILENO;
    allocate_connectionbufs(con_sz, con_buf);

    return 1;
}

void child(int* child_fds) {
    assert(child_fds);
    char* child_buf = (char*) calloc(CLDBUFSIZE, sizeof(char));
    assert(child_buf);
    int rd = 0;

    do {
        ASSERT(child_read, (rd = read(child_fds[0], child_buf, CLDBUFSIZE)) >= 0);
        ASSERT(child_write, write(child_fds[1], child_buf, rd) == rd);
    } while(rd != 0);

    close(child_fds[0]);
    close(child_fds[1]);
    free(child_buf);
}

void server(connection_t* con_buf, int con_sz) {
    fd_set writefds, readfds;

    for(;;) {
        KILL_CHILD
        FD_ZERO(&writefds);
        FD_ZERO(&readfds);

        int maxfd = -230;

        for(int i = 0; i < con_sz; i++) {
            if(con_buf[i].size == 0 && !con_buf[i].dead) {
                FD_SET(con_buf[i].fds[0], &readfds);
                maxfd = MAX(maxfd, con_buf[i].fds[0]);
            }
            if(con_buf[i].size != 0) {
                FD_SET(con_buf[i].fds[1], &writefds);
                maxfd = MAX(maxfd, con_buf[i].fds[1]);
            }
        }

        if(maxfd == -230)
            break;

        ASSERT(select, select(maxfd + 1, &readfds, &writefds, NULL, NULL));

        for(int i = 0; i < con_sz; i++) {
            if(FD_ISSET(con_buf[i].fds[0], &readfds)) {
                int rd = read(con_buf[i].fds[0], con_buf[i].buf, con_buf[i].buf_capacity);
                ASSERT(read, rd >= 0);

                if(rd == 0) {
                    con_buf[i].dead 	= 1;
                    con_buf[i].size 	= 0;
                    close(con_buf[i].fds[1]);
                } else
                    con_buf[i].size     = rd;

                PRINTF("in parent read form %d ; %d connection\n", con_buf[i].fds[0], i);
                PRINTF("rd: %d size: %d offset: %d dead: %d\n", rd, con_buf[i].size, con_buf[i].offset, con_buf[i].dead);
            }

            if(FD_ISSET(con_buf[i].fds[1], &writefds)) {
                int wr = write(con_buf[i].fds[1],con_buf[i].buf + con_buf[i].offset,
                        con_buf[i].size);
                ASSERT(write, wr >= 0);
                con_buf[i].size     -= wr;
                con_buf[i].offset   = (con_buf[i].size == 0) ? 0 : con_buf[i].offset + wr;


                PRINTF("in parent write to %d connection\n", i);
                PRINTF("size: %d offset: %d\n", con_buf[i].size, con_buf[i].offset);
            }
        }
    }
}