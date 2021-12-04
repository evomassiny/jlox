CC=gcc
CFLAGS=-W -I.


value.o: value.c value.h memory.h common.h
	$(CC) -c -o $@ $< -W $(CFLAGS)

debug.o: debug.c debug.h chunk.h common.h
	$(CC) -c -o $@ $< -W $(CFLAGS)

chunk.o: chunk.c chunk.h memory.h common.h value.h
	$(CC) -c -o $@ $< -W $(CFLAGS)

memory.o: memory.c memory.h common.h
	$(CC) -c -o $@ $< -W $(CFLAGS)

clox: main.c chunk.o memory.o debug.o value.o
	$(CC) -o $@ main.c chunk.o memory.o debug.o value.o -W $(CFLAGS)

clean:
	rm clox *.o
