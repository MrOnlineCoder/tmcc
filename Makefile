CC=gcc
CFLAGS=-Wno-nullability-completeness -Iinclude -g
LDFLAGS=

OBJS=obj/lexer.o obj/main.o obj/error.o obj/parser.o obj/ast.o obj/semantic.o obj/codegen.o obj/linker.o obj/types.o obj/symtable.o

prepare:
	mkdir -p obj/

clean:
	rm -rf obj/ tmcc

obj/lexer.o: src/lexer.c include/tmcc/lexer.h include/tmcc/tokens.h
	$(CC) $(CFLAGS) -c src/lexer.c -o obj/lexer.o

obj/main.o: src/main.c include/tmcc/error.h
	$(CC) $(CFLAGS) -c src/main.c -o obj/main.o

obj/error.o: src/error.c include/tmcc/error.h
	$(CC) $(CFLAGS) -c src/error.c -o obj/error.o

obj/ast.o: src/ast.c include/tmcc/ast.h
	$(CC) $(CFLAGS) -c src/ast.c -o obj/ast.o

obj/parser.o: src/parser.c include/tmcc/parser.h include/tmcc/lexer.h include/tmcc/ast.h
	$(CC) $(CFLAGS) -c src/parser.c -o obj/parser.o

obj/semantic.o: src/semantic.c include/tmcc/semantic.h include/tmcc/parser.h include/tmcc/ast.h
	$(CC) $(CFLAGS) -c src/semantic.c -o obj/semantic.o

obj/codegen.o: src/codegen.c include/tmcc/codegen.h include/tmcc/semantic.h include/tmcc/parser.h include/tmcc/ast.h
	$(CC) $(CFLAGS) -c src/codegen.c -o obj/codegen.o

obj/linker.o: src/linker.c include/tmcc/linker.h include/tmcc/codegen.h
	$(CC) $(CFLAGS) -c src/linker.c -o obj/linker.o

obj/types.o: src/types.c include/tmcc/types.h
	$(CC) $(CFLAGS) -c src/types.c -o obj/types.o

obj/symtable.o: src/symtable.c include/tmcc/symtable.h
	$(CC) $(CFLAGS) -c src/symtable.c -o obj/symtable.o

tmcc: prepare $(OBJS) 
	$(CC) $(OBJS) -o tmcc $(LDFLAGS)