all: shmem debug check_sem delete_resource

shmem:
	gcc -g -Wall main.c -o bin/shmem

debug:
	gcc -g -Wall -D _DEBUG main.c -o bin/debug

check_sem:
	gcc -Wall check_sems.c -o bin/check

delete_resource:
	gcc -Wall delete_resource.c -o bin/delete_resource

clear:
	rm -rf bin/*