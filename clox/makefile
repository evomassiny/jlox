CC=gcc
CFLAGS=-W -I.
OBJ=obj


$(OBJ):
	mkdir -p $(OBJ)

$(OBJ)/vm.o: vm.c vm.h memory.h common.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/value.o: value.c value.h memory.h common.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/debug.o: debug.c debug.h chunk.h common.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/chunk.o: chunk.c chunk.h memory.h common.h value.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/memory.o: memory.c memory.h common.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/compiler.o: compiler.c compiler.h common.h scanner.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/scanner.o: scanner.c scanner.h common.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

clox: main.c $(OBJ)/chunk.o $(OBJ)/memory.o $(OBJ)/debug.o $(OBJ)/value.o $(OBJ)/vm.o $(OBJ)/compiler.o $(OBJ)/scanner.o $(OBJ)
	$(CC) -o $@ main.c $(OBJ)/chunk.o $(OBJ)/memory.o $(OBJ)/debug.o $(OBJ)/value.o $(OBJ)/vm.o $(OBJ)/compiler.o $(OBJ)/scanner.o -W $(CFLAGS)

clean:
	rm $(OBJ)/clox $(OBJ)/*.o
