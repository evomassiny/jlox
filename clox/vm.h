#ifndef CLOX_WM_H
#define CLOX_WM_H

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX 256

typedef struct {
  ObjClosure *closure;
  // the instruction we're about to execute, not the one we're executing.
  uint8_t *ip;
  Value *slots; // point to somewhere in the global vm.stack, at the moment of
                // the call, it point to the end of the caller stack
} CallFrame;

typedef struct {
  // Stack frames, grows when calling into a closure/method
  CallFrame frames[FRAMES_MAX];
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
  // number of objects ref in the `grayStack`
  int grayCount;
  // capacity of the gray stack
  int grayCapacity;
  // stack of object's references we are marking (in GC)
  Obj **grayStack;
  // Head of UpValues linked list
  ObjUpvalue *openUpvalues;
  // keeps tracks of memory consumption
  size_t bytesAllocated;
  // when `bytesAllocated` crosses that treshold,
  // trigger a GC cycle.
  size_t nextGC;
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
