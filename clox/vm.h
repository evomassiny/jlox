#ifndef CLOX_WM_H
#define CLOX_WM_H

#include "chunk.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
  // Code + data + debug symbols
  Chunk *chunk;
  // the instruction we're about to execute, not the one we're executing.
  uint8_t *ip;
  // stores evaluated values
  Value stack[STACK_MAX];
  // stack pointer, points to next empty value
  Value *stackTop;
  // interned strings
  Table strings;
  // Head of the heap object linked list
  Obj *objects;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

// export vm globally
extern VM vm;

void initVM();
void freeVM();

InterpretResult interpret(const char *source);
void push(Value value);
Value pop();

#endif
