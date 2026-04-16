CC=gcc
CFLAGS=-Wno-nullability-completeness -Iinclude -g
LDFLAGS=

OBJS=obj/lexer.o obj/main.o obj/error.o

prepare:
	mkdir -p obj/

obj/lexer.o: src/lexer.c include/tmcc/lexer.h
	$(CC) $(CFLAGS) -c src/lexer.c -o obj/lexer.o

obj/main.o: src/main.c 
	$(CC) $(CFLAGS) -c src/main.c -o obj/main.o

obj/error.o: src/error.c include/tmcc/error.h
	$(CC) $(CFLAGS) -c src/error.c -o obj/error.o

tmcc: prepare $(OBJS) 
	$(CC) $(OBJS) -o tmcc $(LDFLAGS)