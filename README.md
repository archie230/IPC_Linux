# MIPT 3rd semester CS course

## Task 1. fifomessenger
Write two programs, which have no parent-child relationship(i.e. one
of them doesn't `fork` and `execve` another when executed), and can be
executed in arbitrary order from different terminals.

***Producer*** process opens, reads the file and passes it to ***consumer*** via
**FIFO**, then ***consumer*** prints file contents to the screen.

***Requirements:***
0. Deadlocks are prohibited.
1. There can be arbitrary number of ***producers*** and ***consumers***.
2. Usage of file locks, semaphores, signals(even handling) and other
communication and synchronization facilities but FIFO is not permitted.
3. Since no blocking synchronization syscalls must be present in
the program, one busy wait is allowed.
4. If after having a handshake ***producer*** or ***consumer*** dies, his
***partner*** should die too, and other ***producers***/***consumers*** should
not be prevented from transmitting/receiving.
5. If both ***partners*** die simultaneously(e.g. computer resets),
other ***producers***/***consumers*** should continue transmitting/receiving.

## Task 2. msgqueue

Write program that produces N children(N is the command line argument).
Then, after all of them are born, they print their numbers(in order of appearence).

3. Only System V IPC message queues are allowed to be used. The queue
must be removed after the program is executed.


## Task 3. shmem
Write two programs, which have no parent-child relationship(i.e. one
of them doesn't `fork` and `execve` another when executed), and can be
executed in arbitrary order from different terminals.

***Producer*** program opens, reads the file and passes it to ***consumer*** via
**shared memory**, then ***consumer*** prints file contents to the screen.

***Requirements:***
0. Deadlocks are prohibited.
1. There can be arbitrary number of ***producers*** and ***consumers***.
2. Usage of any kind of communication and synchronization facilities but
System V IPC **semaphores** and **shared memory** is not permitted.
3. No busy waits are allowed.
4. Any ***producer*** or ***consumer*** can die at any time, this should cause
***partner*** to die, but not prevent any other producers or consumers from
transmitting/receiving data.

## Task 4. signals
Write program that produces child.

Then child process opens file and transmits its contents to the parent. Parent
prints them to the screen.

***Requirements:***
1. Only UNIX signals `SIGUSR1`, `SIGUSR2` are allowed to be used.
2. If one of interlocutors dies, another should die too.

## Task 5. proxy
Write program that produces *N* childs-"***slaves***"(*N* is the command line argument).
Each ***slave*** have buffer of size *X* bytes, and parent-***master*** has N buffers of size
*X \* 3^(N - i)* bytes, where *i* is the number of the buffer.

***Slave*** #0 opens file, then reads it and passes its contents to the parent-***master***.
***Master*** receives data in buffer #0(of the biggest size), then passes it to ***slave*** #1, then
receives it back in buffer #1 and so on.

The point is that the size of ***parent`s*** buffers decreases as the file contents goes towards the final
destination - ***slave*** #N, which prints what it receives to the screen.

***Requirements:***
1. Only **pipes** and `select()` syscall are allowed to be used as the communication and
synchronization facilities.
2. If one of ***slaves*** that didn't finish their work or ***master*** dies, transmission should stop
and all other processes should die.
3. No busy waites nor deadlocks are allowed.
