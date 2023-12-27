CC = gcc
CFLAGS = -Iinclude

SRCS = src/main.c
OBJS = obj/main.o

obj/main.o:
	$(CC) $(CFLAGS) -c src/main.c -o obj/main.o

prepare:
	mkdir -p obj

build: prepare $(OBJS)
	$(CC) $(CFLAGS) -o tmcc $(OBJS)
