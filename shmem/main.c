#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#ifdef  _DEBUG
#define PRINTF printf
#else
#define PRINTF(...)
#endif

enum SEMAPHORES {
    CONSUMER_BUISNESS,
    PRODUCER_BUISNESS,  // 0 - no producer(if no producer+consumer), 1 - one prdoucer
    MUTEX,              // 0 - can entry, 1 - busy
    DATA,               // 0 - empty, 1 - full
    nsems
};

const int sops_sz = 16;    // size of sem operations buf
const int key     = 0xEDA; // key to success
const int shm_sz  = 4096;  // shmem size

int producer(const char*, int, void*);
int consumer(int, void*);

int main(int argc, const char* argv[]) {
    if((argc != 1) && (argc !=2)) {
        errno = EINVAL;
        perror("");
        exit(EXIT_FAILURE);
    }

    int semid = semget(key, nsems, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(key, shm_sz, IPC_CREAT | 0666);
    if(shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    void* shm_ptr = shmat(shmid, NULL, 0);
    if(shm_ptr == (void*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    switch(argc) {
        case 2:
            if (producer(argv[1], semid, shm_ptr) < 0)
                exit(EXIT_FAILURE);
            break;
        case 1:
            if (consumer(semid, shm_ptr) < 0)
                exit(EXIT_FAILURE);
        default:
            errno = EINVAL;
            exit(EXIT_FAILURE);
    }
}

// push operation in sops buf
#define SEMOP_PUSH(_sem_num, _sem_op, _sem_flg)     do {\
                                                    sops[nsops].sem_num  = _sem_num; \
                                                    sops[nsops].sem_op   = _sem_op; \
                                                    sops[nsops].sem_flg  = _sem_flg; \
                                                    nsops++;\
                                                    } while(0)
// do semop with sops cmds and pop all of them
#define SEMOP_POPA()                                ({\
                                                    int _status = 0;\
                                                    assert(nsops < sops_sz);\
                                                    _status = semop(semid, sops, nsops);\
                                                    nsops = 0;\
                                                    _status;})

#ifdef _DEBUG
#define KILL_ME do {printf("KILL ME"); getchar();} while(0);
#else
#define KILL_ME
#endif


int producer(const char* data, int semid, void* shm_ptr) {
    int data_fd = open(data, O_RDONLY);
    if(data_fd < 0) {
        perror("data file open");
        return -1;
    }

    struct sembuf sops[sops_sz];
    int nsops = 0;

    SEMOP_PUSH(PRODUCER_BUISNESS, 0, 0);
    SEMOP_PUSH(CONSUMER_BUISNESS, 0, 0);
    SEMOP_PUSH(PRODUCER_BUISNESS, 2, SEM_UNDO);
    SEMOP_PUSH(PRODUCER_BUISNESS, -1, 0);
    SEMOP_POPA();

    SEMOP_PUSH(CONSUMER_BUISNESS, -1, 0);
    SEMOP_PUSH(CONSUMER_BUISNESS, 0, 0);
    SEMOP_PUSH(CONSUMER_BUISNESS,1, 0);
    SEMOP_POPA();

    int32_t readed = 0;
    do {
        SEMOP_PUSH(CONSUMER_BUISNESS, -1, IPC_NOWAIT);
        SEMOP_PUSH(CONSUMER_BUISNESS, 0, IPC_NOWAIT);
        SEMOP_PUSH(CONSUMER_BUISNESS, 1, 0);
        SEMOP_PUSH(DATA, 0, 0);
        SEMOP_PUSH(MUTEX, 0, 0);
        SEMOP_PUSH(MUTEX, 1, SEM_UNDO);
        if(SEMOP_POPA() < 0) {
            perror("consumer zdoh");
            return -1;
        }

        readed = read(data_fd, shm_ptr + sizeof(int32_t), shm_sz - sizeof(int32_t));
        if(readed < 0) {
            perror("data file read");
            return -1;
        }

        *((int32_t*) shm_ptr) = readed;

        SEMOP_PUSH(DATA, 1, 0);
        SEMOP_PUSH(MUTEX, -1, SEM_UNDO);
        SEMOP_PUSH(MUTEX, 0 , 0);
        if(SEMOP_POPA() < 0) {
            perror("someone has changed mutex semaphore!");
            return -1;
        }
        KILL_ME
    } while(readed != 0);

    SEMOP_PUSH(CONSUMER_BUISNESS, 0, 0);
    SEMOP_PUSH(PRODUCER_BUISNESS, -2, SEM_UNDO);
    SEMOP_POPA();

    return 0;
}

int consumer(int semid, void* shm_ptr) {
    struct sembuf sops[sops_sz];
    int nsops = 0;

    SEMOP_PUSH(PRODUCER_BUISNESS, -1, 0);
    SEMOP_PUSH(PRODUCER_BUISNESS, 0, 0);
    SEMOP_PUSH(PRODUCER_BUISNESS, 2, 0);
    SEMOP_PUSH(CONSUMER_BUISNESS, 0 , 0);
    SEMOP_PUSH(CONSUMER_BUISNESS, 1, SEM_UNDO);
    SEMOP_PUSH(DATA, 1, SEM_UNDO);
    SEMOP_PUSH(DATA, -1, SEM_UNDO);
    SEMOP_POPA();

    int32_t readed = 0;

    do {
        KILL_ME
        SEMOP_PUSH(PRODUCER_BUISNESS, -2, IPC_NOWAIT);
        SEMOP_PUSH(PRODUCER_BUISNESS, 0, IPC_NOWAIT);
        SEMOP_PUSH(PRODUCER_BUISNESS, 2, 0);
        SEMOP_PUSH(DATA, -1, 0);
        SEMOP_PUSH(DATA, 0, IPC_NOWAIT);
        SEMOP_PUSH(MUTEX, 0, 0);
        SEMOP_PUSH(MUTEX, 1, SEM_UNDO);
        if(SEMOP_POPA() < 0) {
            perror("producer zdoh");
            return -1;
        }

        readed = *((int32_t*) shm_ptr);
        if(readed < 0) {
            perror("producer ploho napisal");
            return -1;
        }

        if(write(STDOUT_FILENO, shm_ptr + sizeof(int32_t), readed) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        SEMOP_PUSH(MUTEX, -1, SEM_UNDO);
        SEMOP_PUSH(MUTEX, 0, IPC_NOWAIT);
        if(SEMOP_POPA() < 0) {
            perror("MUTEX semaphore isporchen");
            return -1;
        }
    } while(readed > 0);

    SEMOP_PUSH(CONSUMER_BUISNESS, -1, SEM_UNDO);
    SEMOP_POPA();

    return 0;
}

#undef SEMOP_POPA
#undef SEMOP_PUSH