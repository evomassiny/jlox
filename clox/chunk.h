#ifndef clox_chunk_h
#define clox_chunk_h
#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_ADD,
  OP_SUBSTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
  OP_RETURN,
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
