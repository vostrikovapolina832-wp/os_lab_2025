
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -I../../lab3/src
AR = ar

.PHONY: all clean

all: parallel_min_max zombie_demo process_memory psum

parallel_min_max: parallel_min_max.c ../../lab3/src/find_min_max.c ../../lab3/src/utils.c
	$(CC) $(CFLAGS) -o $@ parallel_min_max.c ../../lab3/src/find_min_max.c ../../lab3/src/utils.c

zombie_demo: zombie_demo.c
	$(CC) $(CFLAGS) -o $@ zombie_demo.c

process_memory: process_memory.c
	$(CC) $(CFLAGS) -o $@ process_memory.c

psum: parallel_sum.c libsum.a ../../lab3/src/utils.c
	$(CC) $(CFLAGS) -pthread -o $@ parallel_sum.c ../../lab3/src/utils.c -L. -lsum

sum.o: sum.c sum.h
	$(CC) $(CFLAGS) -pthread -c sum.c -o $@

libsum.a: sum.o
	$(AR) rcs $@ sum.o

clean:
	rm -f parallel_min_max zombie_demo process_memory psum sum.o libsum.a
