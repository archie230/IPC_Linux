all: proxy tests debug

proxy:
	cc -g -Wall proxy.c -lm -o bin/proxy

tests:
	cc -g -Wall -D _TEST proxy.c -lm -o bin/test

debug:
	cc -g -Wall -D _DEBUG proxy.c -lm -o bin/debug

clear:
	rm -r bin/*