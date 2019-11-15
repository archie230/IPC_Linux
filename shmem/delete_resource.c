#include <stddef.h>
#include "sys/shm.h"
#include "sys/sem.h"

const int KEY = 0xEDA;

int main() {
    shmctl(0xEDA, IPC_RMID, NULL);
    semctl(0xEDA, 0, IPC_RMID);

    return 0;
}