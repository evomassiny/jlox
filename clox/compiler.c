#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char* source) {
    initScanner(source);

    int line = -1;
    for (;;) {
        Token token = scanToken();
        if (token.line != line) {
            line = token.line;
            printf("%4d ", token.line);
        } else {
            printf("    |");
        }

        // because `token.start` might not be NULL termnated, 
        // we need to specify the length to display,
        // we do  this using (`%.3s`, str),
        // or we can specify it dynamically using ('%.*s', 3, str)
        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TOKEN_EOF) break;
    }
}
