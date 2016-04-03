main: sh.c
	gcc -g sh.c

clean:
	rm -f sh a.out

gdb: clean main
	gdb -iex "set follow-fork-mode child" a.out

run: clean main
	./a.out
