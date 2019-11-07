
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                           (Linux-specific) */
};


int main(int argc, char* argv[]) {
    int semid = semget(0xEDA, 4, 0);
    if(semid < 0) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    unsigned short* buf = (unsigned short*) calloc(4, sizeof(unsigned short));
    buf[0] = -1;
    buf[1] = -1;
    buf[2] = -1;
    buf[3] = -1;
    union semun arg = {.array=buf};

    if(semctl(semid, 0,  GETALL, arg) < 0) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    printf("CONSUMER_BUISNESS = %d\n", buf[0]);
    printf("PRODUCER_BUISNESS = %d\n", buf[1]);
    printf("MUTEX = %d\n", buf[2]);
    printf("DATA = %d\n", buf[3]);

    if(argc == 2) {
        for(int i = 0; i < 4; i++)
            buf[i] = 0;

        if(semctl(semid, 0, SETALL, arg) < 0) {
            perror("semctl SETALL");
            exit(EXIT_FAILURE);
        }
    }

    free(buf);
    return 0;
}
