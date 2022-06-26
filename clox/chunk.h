#ifndef clox_chunk_h
#define clox_chunk_h
#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_NIL, // push `nil` on the execution stack (this avoid storing it within the
          // constant table).
  OP_TRUE,      // same for `true`
  OP_FALSE,     // same for `false`
  OP_POP,       // drop last inserted stack entry
  OP_GET_LOCAL, // load from stack
  OP_SET_LOCAL,
  OP_GET_GLOBAL, // load from global hastable
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_UPVALUE, // load from closure `upvalue` (either stored directly in the
                  // array or referenced by it)
  OP_SET_UPVALUE,
  OP_GET_PROPERTY, // load from ?
  OP_SET_PROPERTY,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBSTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP, // backward jump
  OP_CALL,
  OP_INVOKE,        // call a method (bound to an object)
  OP_CLOSURE,       // push closure onto stack
  OP_CLOSE_UPVALUE, // move local variable onto heap, so it can outlive the its
                    // stackframe, and be referred by closures.
  OP_RETURN,
  OP_CLASS,
  OP_METHOD, // bind a method to a class object
} OpCode;

// holds TEXT + DATA + DEBUG info
typedef struct {
  int count; // length of `code` and `lines`
  int capacity;
  uint8_t *code; // can either hold instruction OR their operand (ref indices to
                 // constant)
  int *lines;    // line nb of each `code` instruction
  ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);

#endif
