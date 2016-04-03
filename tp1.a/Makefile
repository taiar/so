main: sh.c
	gcc -g sh.c -o sh

clean:
	rm -f sh a.out

gdb: clean main
	gdb -iex "set follow-fork-mode child" sh

run: clean main
	./sh
