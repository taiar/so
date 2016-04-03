clean:
	rm -f sh a.out

main: sh.c
	gcc -g sh.c -o sh

dbg: clean main
	gdb -iex "set follow-fork-mode child" sh

run: clean main
	./sh
