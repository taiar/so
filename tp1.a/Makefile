main: sh.c
	gcc -g sh.c

test: main
	./a.out < teste.sh

simpletest: main
	./a.out < simple_test.sh

clean:
	rm -f sh a.out

gdb: clean main
	gdb -iex "set follow-fork-mode child" a.out

run: clean main
	./a.out
