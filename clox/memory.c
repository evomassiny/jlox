#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include "debug.h"
#include <stdio.h>
#endif

#define GC_HEAP_GROW_FACTOR 2

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
  // keep track of allocated memory
  vm.bytesAllocated += newSize - oldSize;
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif
    if (vm.bytesAllocated > vm.nextGC) {
      collectGarbage();
    }
  }

  if (newSize == 0) {
    free(pointer);
    return NULL;
  }
  // NOTE: if pointer == NULL, this is similar to malloc()
  void *result = realloc(pointer, newSize);
  if (result == NULL) {
    exit(1);
  }
  return result;
}

// mark an object, and append it to the vm's gray object stack
void markObject(Obj *object) {
  if (object == NULL) {
    return;
  }
  if (object->isMarked) {
    return;
  }
#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void *)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

  object->isMarked = true;

  if (vm.grayCapacity < vm.grayCount + 1) {
    vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
    // reallocate using system realloc, to avoid trigerring
    // the GC while garbage collecting.
    vm.grayStack =
        (Obj **)realloc(vm.grayStack, sizeof(Obj *) * vm.grayCapacity);
    // panic if alloaction fails
    if (vm.grayStack == NULL) {
      exit(1);
    }
  }

  vm.grayStack[vm.grayCount++] = object;
}

void markValue(Value value) {
  if (IS_OBJ(value))
    markObject(AS_OBJ(value));
}

void markArray(ValueArray *array) {
  for (int i = 0; i < array->count; i++) {
    markValue(array->values[i]);
  }
}

/**
 * Append the children of the object
 * to the "grayStack", unless they are "marked" (done in markObject)
 * returning from this function effectively un-queue
 * the object from the "gray" list.
 */
static void blackenObject(Obj *object) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void *)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  switch (object->type) {
  // mark class bound object (ObjInstance wrapped in Value) + method
  // (ObjClosure)
  case OBJ_BOUND_METHOD: {
    ObjBoundMethod *bound = (ObjBoundMethod *)object;
    markValue(bound->receiver);
    markObject((Obj *)bound->method);
    break;
  }
  // mark class name + methods
  case OBJ_CLASS: {
    ObjClass *klass = (ObjClass *)object;
    markObject((Obj *)klass->name);
    markTable(&klass->methods);
    break;
  }
  // mark function and the upvalues
  case OBJ_CLOSURE: {
    ObjClosure *closure = (ObjClosure *)object;
    markObject((Obj *)closure->function);
    for (int i = 0; i < closure->upvalueCount; i++) {
      markObject((Obj *)closure->upvalues[i]);
    }
    break;
  }
  // mark the name and the constant table of the function
  case OBJ_FUNCTION: {
    ObjFunction *function = (ObjFunction *)object;
    markObject((Obj *)function->name);
    markArray(&function->chunk.constants);
    break;
  }
  // mark klass and fields
  case OBJ_INSTANCE: {
    ObjInstance *instance = (ObjInstance *)object;
    markObject((Obj *)instance->klass);
    markTable(&instance->fields);
    break;
  }
  // simply mark the value
  case OBJ_UPVALUE:
    markValue(((ObjUpvalue *)object)->closed);
    break;
  // contains no outgoing references => NOP
  case OBJ_NATIVE:
  // contains no outgoing references => NOP
  case OBJ_STRING:
    break;
  }
}

void freeObject(Obj *object) {
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void *)object, object->type);
#endif
  switch (object->type) {
  case OBJ_BOUND_METHOD: {
    FREE(ObjBoundMethod, object);
    // does not _own_ the method nor the object bound to it.
    break;
  }
  case OBJ_CLASS: {
    // We rely on garbage collection to free `class->name`
    ObjClass *klass = (ObjClass *)object;
    freeTable(&klass->methods);
    FREE(ObjClass, object);
    break;
  }
  case OBJ_CLOSURE: {
    // closure does not own its function
    ObjClosure *closure = (ObjClosure *)object;
    FREE_ARRAY(ObjUpvalue *, closure->upvalues, closure->upvalueCount);
    FREE(ObjClosure, object);
    break;
  }
  case OBJ_UPVALUE: {
    FREE(ObjUpvalue, object);
    break;
  }
  case OBJ_FUNCTION: {
    // downcast Obj -> ObjFunction
    ObjFunction *function = (ObjFunction *)object;
    freeChunk(&function->chunk);
    FREE(ObjFunction, object);
    // We rely on garbage collection to free `function->name`
    break;
  }
  case OBJ_INSTANCE: {
    // We rely on garbage collection to free `instance->klass`
    ObjInstance *instance = (ObjInstance *)object;
    freeTable(&instance->fields);
    FREE(ObjInstance, object);
    break;
  }
  case OBJ_NATIVE: {
    FREE(ObjNative, object);
    break;
  }
  case OBJ_STRING: {
    // downcast Obj -> ObjString
    ObjString *string = (ObjString *)object;
    FREE_ARRAY(char, string->chars, string->length + 1);
    FREE(ObjString, object);
    break;
  }
  }
}

// mark traversed objects, starting from the roots
// (part of GC mark phase)
static void markRoots(void) {
  // check vm value stack
  for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
    markValue(*slot);
  }

  // check closures
  for (int i = 0; i < vm.frameCount; i++) {
    markObject((Obj *)vm.frames[i].closure);
  }

  // check upvalues
  for (ObjUpvalue *upvalue = vm.openUpvalues; upvalue != NULL;
       upvalue = upvalue->next) {
    markObject((Obj *)upvalue);
  }

  // check globals
  markTable(&vm.globals);

  // mark objects allacated by the compiler
  markCompilerRoots();

  // interned "init" string
  markObject((Obj *)vm.initString);
}

static void traceReferences(void) {
  // process the "grayStack", as a process queue
  while (vm.grayCount > 0) {
    Obj *object = vm.grayStack[--vm.grayCount];
    // might grow the grayStack
    blackenObject(object);
  }
}

static void sweep(void) {
  Obj *previous = NULL;
  Obj *object = vm.objects;

  while (object != NULL) {
    if (object->isMarked) {
      // clear marked flag for next GC cycle
      object->isMarked = false;
      // go to next obj
      previous = object;
      object = object->next;
    } else {
      // update head or link to previous
      Obj *unreached = object;
      object = object->next;

      if (previous != NULL) {
        previous->next = object;
      } else {
        vm.objects = object;
      }

      freeObject(unreached);
    }
  }
}

void collectGarbage(void) {
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = vm.bytesAllocated;
#endif
  markRoots();
  traceReferences();
  // strings are interned in a HashMap, as KEYS.
  // The global vm.strings hashmap stores all allocated string pointers
  // in a table `Entry` (key + value, here `pointer to string` + NIL).
  // When we de-allocate a key, we must also remove the entry from
  // the table. Otherwise the key would contain a dangling pointer to
  // a non-existing string.
  tableRemoveWhite(&vm.strings);
  sweep();

  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
#endif
}

void freeObjects(void) {
  Obj *obj = vm.objects;
  while (obj != NULL) {
    Obj *next = obj->next;
    freeObject(obj);
    obj = next;
  }
  free(vm.grayStack);
}
