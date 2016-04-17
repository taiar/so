main: thread
	gcc main.c -o thread

clean:
	rm -rf *.o thread

thread:
	gcc -c dccthread.c

run: main
	./thread

teste1: thread
	gcc teste1.c dccthread.o -o thread
	./thread
