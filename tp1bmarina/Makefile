main: thread
	gcc main.c -o thread -g -lm -Wall

clean:
	rm -rf *.o thread

thread:
	gcc -c dccthread.c -g -lm -Wall

run: main
	./thread

teste1: thread
	gcc teste1.c dccthread.o -o thread -g -lm -Wall
	./thread

teste2: thread
	gcc teste2.c dccthread.o -o thread -g -lm -Wall
	./thread