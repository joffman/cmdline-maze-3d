# *- Makefile -*

# variables
CC = gcc
CFLAGS = -Wall -std=c99 -c -g
LDFLAGS = -lncursesw -lm

# target: dependencies
# 	actions

all: main

main: main.o
	$(CC) main.o $(LDFLAGS) -o main

main.o: main.c edge.h
	$(CC) $(CFLAGS) main.c


clean:
	rm -f *.o main
