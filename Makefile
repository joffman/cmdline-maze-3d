# *- Makefile -*

# variables
CC = gcc
CFLAGS = -Wall -ansi -c -g
LDFLAGS = 

# target: dependencies
# 	actions

all: main

main: main.o
	$(CC) $(LDFLAGS) main.o -o main

main.o: main.c
	$(CC) $(CFLAGS) main.c


clean:
	rm -f *.o main
