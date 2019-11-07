all: shmem debug check_sem

shmem:
	gcc -g -Wall main.c -o bin/shmem

debug:
	gcc -g -Wall -D _DEBUG main.c -o bin/debug

check_sem:
	gcc -g -Wall check_sems.c -o bin/check

clear:
	rm -rf bin/*