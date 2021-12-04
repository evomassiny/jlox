CC=gcc
CFLAGS=-W -I.

chunk.o: chunk.c chunk.h memory.h
	$(CC) -c -o $@ $< -W $(CFLAGS)

memory.o: memory.c memory.h
	$(CC) -c -o $@ $< -W $(CFLAGS)

clox: main.c chunk.o memory.o
	$(CC) -o $@ main.c chunk.o memory.o -W $(CFLAGS)

clean:
	rm clox clox.o
