main: thread
	gcc main.c -o thread

clean:
	rm -rf *.o thread

thread:
	gcc -c dccthread.c

run: main
	./thread

teste1: main
	gcc teste1.c -o thread
	run
