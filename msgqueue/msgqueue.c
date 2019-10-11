#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>

struct msg {
    long type;
};

enum ERRS {
    NotNum = -230,
    LongOverflow = -231
};

long get_number(const char*);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        errno = EINVAL;
        perror("");
        exit(EXIT_FAILURE);
    }

    long child_number = get_number(argv[1]);
    if (child_number < 0) {
        errno = EINVAL;
        perror("");
        exit(EXIT_FAILURE);
    }

    //creating msgqueue
    int msqid = msgget(IPC_PRIVATE, 0666);
    if (msqid < 0) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }


    pid_t pid = -230;
    struct msg ms = {0};

    for (int i = 1; i <= child_number; i++) {
        pid = fork();
        if (pid == 0) {
            if (i != 1)
                if (msgrcv(msqid, &ms, 0, i - 1, 0) < 0) {
                    perror("msgrcv");
                    exit(EXIT_FAILURE);
                }

            printf("%d ", i);
            fflush(stdout);

            ms.type++;
            if (msgsnd(msqid, &ms, 0, 0) < 0) {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }
    if (msgrcv(msqid, &ms, 0, child_number, 0) < 0) {
        perror("msgrcv");
        exit(EXIT_FAILURE);
    }

    if(msgctl(msqid, IPC_RMID, NULL) < 0) {
        perror("msgctl");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < child_number; i++)
        wait(NULL);

    return 0;
}

long get_number(const char* str) {
    char* str_end;
    long number = strtol(str, &str_end, 10);

    if (errno == ERANGE && (number == LONG_MAX || number == LONG_MIN))
        return LongOverflow;
    if ((str_end == str) || (errno != 0 && number == 0))
        return NotNum;

    return number;
}