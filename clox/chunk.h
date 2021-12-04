#ifndef clox_chunk.h 
#define clox_chunk.h 
#include "common.h"



typedef enum {
    OP_RETURN,
} OpCode;

// basically a Vec
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);

#endif
