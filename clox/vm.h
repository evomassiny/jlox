#ifndef CLOX_WM_H
#define CLOX_WM_H

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    // Code + data + debug symbols
    Chunk* chunk;
    // the instruction we're about to execute, not the one we're executing.
    uint8_t* ip;
    // stores evaluated values
    Value stack[STACK_MAX];
    // stack pointer, points to next empty value
    Value* stackTop;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void initVM();
void freeVM();

InterpretResult interpret(Chunk* chunk);
void push(Value value);
Value pop();

#endif
