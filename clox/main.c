#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "vm.h"


static void repl(void) {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        interpret(line);
    }
}

static readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open '%s'", path);
        exit(74);
    }
    // get content length
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* buffer = (char*) malloc(sizeof(char) * fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read '%s'", path);
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';
    if (bytesRead < fileSize) {
        fprintf(stderr, "Failed to read '%s'", path);
        exit(74);
    }

    fclose(file);
    return buffer;
}

static runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, char ** argv) {
    initVM();
    
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: %s clox [path]\n", argv[0]);
    }

    freeVM();
    return 0;
}
