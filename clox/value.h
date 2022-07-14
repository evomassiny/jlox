#ifndef clox_value_h
#define clox_value_h

#include "common.h"
#include <string.h>

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING
/*
 * The following squeeze a type TAG, objects pointers and numbers
 * into 64bits, using a technique called NAN_BOXING.
 *
 * It relies on 2 facts:
 * * pointers (addresses) never actually uses more that 48 bits.
 * * doubles can be "Quiet Nans", if so, highest first bit, and 49 lowest
 *   could be anything. We never use those "quiet nans".
 *
 * The main idea is to distinguish floats and other types by looking if the
 * QNAN bits are set, if so, the value is of type Obj*, bool or Nil.
 *
 * Then if the first value of the highest bit is set, the value is an object
 * pointer,
 * otherwise, we look for the 2 lowest bits to tell appart Nils, from Trues and
 * from Falses.
 */

// first bit set
#define SIGN_BIT ((uint64_t)0x8000000000000000)
// bits of "Quiet NaNs"
#define QNAN ((uint64_t)0x7ffc000000000000)

#define TAG_NIL 1   // b01
#define TAG_FALSE 2 // b10
#define TAG_TRUE 3  // b11

#define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))

typedef uint64_t Value;

// type pun a "Value" to a double,
// assumes that the compiler will optimise it away.
static inline Value numToValue(double num) {
  Value value;
  memcpy(&value, &num, sizeof(double));
  return value;
}

// type pun a "double" to a "Value",
// assumes that the compiler will optimise it away.
static inline double valueToNum(Value value) {
  double num;
  memcpy(&num, &value, sizeof(Value));
  return num;
}

// Type checks
// set the last bit to 1, so if it was a FALSE_VAL it would have been converted
// to a TRUE_VAL
#define IS_BOOL(value) (((value) | 1) == TRUE_VAL)
#define IS_NIL(value) ((value) == NIL_VAL)
#define IS_NUMBER(value) ((value)&QNAN == QNAN)
#define IS_OBJ(value) (((value) & (SIGN_BIT | QNAN)) == (SIGN_BIT | QNAN))

// Values to C types
#define AS_BOOL(value) ((value) == TRUE_VAL)
#define AS_NUMBER(value) ((double)(value))
#define AS_OBJ(value) ((Obj *)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

// C types to Values
#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
#define NUMBER_VAL(num) numToValue(num)
// this works because most (all?) achitectures
// only use the lowest 48 bits to store addresses
#define OBJ_VAL(object_ptr)                                                    \
  ((Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(object_ptr)))

#else

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ,
} ValueType;

// Type of values, those live
// on the VM stack
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj *obj;
  } as;
} Value;

// Type checks
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

// Values to C types
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

// C types to Values
#define BOOL_VAL(b) ((Value){VAL_BOOL, {.boolean = b}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0.}})
#define NUMBER_VAL(num) ((Value){VAL_NUMBER, {.number = num}})
#define OBJ_VAL(object) ((Value){.type = VAL_OBJ, {.obj = (Obj *)object}})

#endif

typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

bool valuesEqual(Value a, Value b);

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);

#endif
