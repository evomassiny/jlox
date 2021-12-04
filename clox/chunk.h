#ifndef clox_chunk_h 
#define clox_chunk_h 
#include "common.h"
#include "value.h"



typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} OpCode;

typedef struct {
    int count;      // length of `code` and `lines`
    int capacity;
    uint8_t* code;  // can either hold instruction OR their operand
    int* lines;      // line nb of each `code` instruction
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

#endif
