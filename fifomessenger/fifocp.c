#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <sys/ioctl.h>

#define _DEBUG

#define PERM_MASK 0644
#define BUF_SIZE PIPE_BUF
#define TIME_LIMIT 4

#ifdef _DEBUG
#define PRINTF printf
#else
#define PRINTF(args, ...)
#endif

const char g_service_fifo[] = "SERVICE_FIFO";


//sending message
void server(const char*);
//accepting message
void client();
int open_service_fifo(int);
int get_namecode(int);
void send_namecode(pid_t, int);
void set_datafifo_name(pid_t, char*);

int main(int argc, char* argv[]) {
    switch(argc) {
        case 1: //client
            client();
            break;
        case 2: //server
            server(argv[1]);
            break;
        default:
            errno = EINVAL;
            perror("max number of arguments = 1!\n");
            exit(EXIT_FAILURE);
    }

    return 0;
}

void server(const char* src) {
    int src_fd = open(src, O_RDONLY);
    if(src_fd < 0) {
        perror("opening source file failed!\n");
        exit(EXIT_FAILURE);
    }

    int srv_fifo_fd = open_service_fifo(O_RDONLY); // O_RDWR ?? O_RDONLY
    int pid = get_namecode(srv_fifo_fd);

    char data_fifo[64] = {};
    set_datafifo_name(pid, data_fifo);

    PRINTF("name: %s\n", data_fifo);

    int data_npipe_fd = open(data_fifo, O_WRONLY | O_NONBLOCK);
    if(data_npipe_fd < 0) {
        fprintf(stderr, "there are problems with client process(pid : %d)\n", pid);
        exit(EXIT_FAILURE);
    }

    if(fcntl(data_npipe_fd, F_SETFL, O_WRONLY) < 0) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    int readed = 0;
    int writed = 0;

    char buf[BUF_SIZE];

    while((readed = read(src_fd, buf, BUF_SIZE)) > 0) {
        if((writed = write(data_npipe_fd, buf, readed)) < 0) {
            perror("server write");
            exit(EXIT_FAILURE);
        }
        assert(writed == readed);
        PRINTF("\nreaded: %d\n", readed);
        PRINTF("writed: %d\n", writed);
    }

    close(src_fd);
    close(srv_fifo_fd);
    close(data_npipe_fd);
}

void client() {
    pid_t pid = getpid();
    char data_fifo[64] = {};

    set_datafifo_name(pid, data_fifo);
    PRINTF("name: %s\n", data_fifo);

    if(mkfifo(data_fifo, PERM_MASK) < 0) {
        perror("client mkfifo");
        exit(EXIT_FAILURE);
    }

    int data_fifo_fd = open(data_fifo, O_RDONLY | O_NONBLOCK);
    if(data_fifo_fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int srv_fifo_fd = open_service_fifo(O_WRONLY); // O_WRONLY ?? O_RDWR
    PRINTF("opened service in client\n");
    send_namecode(pid, srv_fifo_fd);
    PRINTF("wrote pid\n");

    if(fcntl(data_fifo_fd, F_SETFL, O_RDONLY) < 0) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    int nbytes = 0;
    ioctl(data_fifo_fd, FIONREAD, &nbytes);

    if(nbytes == 0)
        sleep(TIME_LIMIT);

    int readed = 0;
    int writed = 0;
    char buf[BUF_SIZE];

    while((readed = read(data_fifo_fd, &buf, BUF_SIZE)) > 0) {
        if((writed = write(STDOUT_FILENO, buf, readed)) < 0) {
            perror("client write");
            exit(EXIT_FAILURE);
        }
        assert(writed == readed);
        PRINTF("\nreaded: %d\n", readed);
        PRINTF("writed: %d\n", writed);
    }

    close(data_fifo_fd);
    close(srv_fifo_fd);
    unlink(data_fifo);
}

int open_service_fifo(int flag) {
    int serv_fifo_fd = mkfifo(g_service_fifo, PERM_MASK);
    if((serv_fifo_fd < 0) && (errno != EEXIST)) {
        fprintf(stderr, "mkfifo %s failed!\n", g_service_fifo);
        exit(EXIT_FAILURE);
    }

    serv_fifo_fd = open(g_service_fifo, flag);
    if(serv_fifo_fd < 0) {
        fprintf(stderr, "can't open %s\n errno: %d\n", g_service_fifo, errno);
        exit(EXIT_FAILURE);
    }
    return serv_fifo_fd;
}

void send_namecode(pid_t pid, int serv_fifo_fd) {
    int writed = write(serv_fifo_fd, &pid, sizeof(pid_t));
    if(writed != sizeof(pid_t)) {
        perror("can't send namecode!\n");
        exit(EXIT_FAILURE);
    }
}

int get_namecode(int serv_fifo_fd) {
    pid_t namecode = 0;
    int readed = read(serv_fifo_fd, &namecode, sizeof(pid_t));
    if(readed != sizeof(pid_t)) {
        fprintf(stderr,"can't read pid; readed %d\n", readed);
        exit(EXIT_FAILURE);
    }

    if(namecode < 0) {
        perror("bad pid readed from SERVICE_FIFO");
        exit(EXIT_FAILURE);
    }

    return namecode;
}

void set_datafifo_name(pid_t pid, char* datafifo) {
    sprintf(datafifo, "DATA_FIFO_%d", pid);
}