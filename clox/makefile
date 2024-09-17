CC=gcc
CFLAGS=-g -W -I.
OBJ=obj
INSTALL_PATH=~/.local/bin


$(OBJ):
	mkdir -p $(OBJ)

$(OBJ)/vm.o: vm.c vm.h common.h compiler.h object.h debug.h  memory.h object.h table.h value.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/object.o: object.c object.h common.h memory.h table.h chunk.h value.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/value.o: value.c value.h object.h memory.h common.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/debug.o: debug.c debug.h object.h chunk.h common.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/chunk.o: chunk.c chunk.h memory.h common.h value.h vm.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/memory.o: memory.c memory.h common.h object.h compiler.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/compiler.o: compiler.c compiler.h common.h scanner.h object.h memory.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/scanner.o: scanner.c scanner.h common.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

$(OBJ)/table.o: table.c table.h common.h memory.h object.h table.h value.h $(OBJ)
	$(CC) -c -o $@ $< -W $(CFLAGS)

clox: main.c $(OBJ)/chunk.o $(OBJ)/memory.o $(OBJ)/debug.o $(OBJ)/value.o $(OBJ)/vm.o $(OBJ)/compiler.o $(OBJ)/scanner.o $(OBJ)/object.o $(OBJ)/table.o
	 $(CC) -o $@ main.c $(OBJ)/chunk.o $(OBJ)/memory.o $(OBJ)/debug.o $(OBJ)/value.o $(OBJ)/vm.o $(OBJ)/compiler.o $(OBJ)/scanner.o $(OBJ)/object.o $(OBJ)/table.o -W $(CFLAGS)

clean:
	rm $(OBJ)/clox $(OBJ)/*.o

format:
	clang-format -i *.h *.c

install: clox
	ln $< $(INSTALL_PATH)/clox

uninstall: 
	rm -rf $(INSTALL_PATH)/clox
