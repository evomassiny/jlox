#ifndef clox_object_h
#define clox_object_h

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_CLOSURE(value) ((ObjClosure *)(value).as.obj)
#define AS_FUNCTION(value) ((ObjFunction *)(value).as.obj)
#define AS_NATIVE(value) (((ObjNative *)(value).as.obj)->function)
#define AS_STRING(value) ((ObjString *)(value).as.obj)
#define AS_CSTRING(value) (((ObjString *)(value).as.obj)->chars)

typedef enum {
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_UPVALUE,
} ObjType;

// mockup inheritance, see ObjFunction, ObjString
struct Obj {
  ObjType type;
  struct Obj *next;
};

typedef struct {
  Obj obj; // because #[repr(C)]: `(obj*) &ObjFunction` is valid.
  int arity;
  int upvalueCount; // number of ref to outer function locals
  Chunk chunk;
  ObjString *name;
} ObjFunction;

// Native function pointers
typedef Value (*NativeFn)(int argCount, Value *args);

// Object wrapping native function
typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

struct ObjString {
  Obj obj;    // because #[repr(C)]: `(obj*) &ObjString` is valid.
  int length; // EXCLUDING trailing '\0'
  char *chars;
  uint32_t hash; // not garanteed to be uniq per string.
};

typedef struct ObjUpvalue {
  Obj obj;
  Value *location;         // point to function stack frame
  struct ObjUpvalue *next; // linked to next upvalue
  Value closed;            // Copy of what used to be a stackframe local
} ObjUpvalue;

typedef struct {
  Obj obj; // because #[repr(C)]: `(obj*) &ObjClosure` is valid.
  ObjFunction *function;
  ObjUpvalue **upvalues; // refs to outer function locals
  int upvalueCount;
} ObjClosure;

ObjFunction *newFunction();
ObjClosure *newClosure(ObjFunction *function);
ObjNative *newNative(NativeFn function);
ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *chars, int length);
ObjUpvalue *newUpvalue(Value *slot);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
  // equivalent to:
  // return value.type == VAL_OBJ && value.as.obj->type == type;
}

#endif
