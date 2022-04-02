#ifndef CLOX_WM_H
#define CLOX_WM_H

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAME_MAX 64
#define STACK_MAX 256

typedef struct {
  ObjFunction *function;
  // the instruction we're about to execute, not the one we're executing.
  uint8_t *ip;
  Value *slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAME_MAX];
  int frameCount;
  // stores evaluated values
  Value stack[STACK_MAX];
  // stack pointer, points to next empty value
  Value *stackTop;
  // Global variables values, by name
  Table globals;
  // ALL interned strings
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
