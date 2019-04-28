# *- Makefile -*

# variables
CC = gcc
CFLAGS = -Wall -std=c99 -c -g
LDFLAGS = -lncurses -lm

# target: dependencies
# 	actions

all: main

main: main.o
	$(CC) main.o $(LDFLAGS) -o main

main.o: main.c
	$(CC) $(CFLAGS) main.c


clean:
	rm -f *.o main
