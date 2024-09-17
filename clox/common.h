#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// print compiled opcodes
// #define DEBUG_PRINT_CODE

// print stack state during execution
// #define DEBUG_TRACE_EXECUTION

// inserts lots of GC passes, in the hope to trigger bugs
// #define DEBUG_STRESS_GC

// print GC degug info, at each pass
// #define DEBUG_LOG_GC

// if set, use Nan Boxing to reduce the size of
// the Value type down to 64 bits.
#define NAN_BOXING

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
