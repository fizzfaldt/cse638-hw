CC=cilk++

a.out: bfs.cc bfs.h Makefile fakecilk.h
	$(CC) -Wall -O0 -g3 -ggdb -Wextra $< -I /usr/include/x86_64-linux-gnu -B /usr/lib/x86_64-linux-gnu -lrt
