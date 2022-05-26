#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

// free it content
void freeTable(Table *table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

/**
 * find entry in flat value array using key's hash.
 * (Implements linear probing)
 *
 */
static Entry *findEntry(Entry *entries, int capacity, ObjString *key) {
  uint32_t index = key->hash % capacity;
  Entry *tombstone = NULL;
  // looks for existing entry or empty slot,
  // `findEntry()` assumes that the Table is NEVER FULL.
  for (;;) {
    Entry *entry = &entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // if after following the chain, we encounter an empty
        // entry, this means that the `key` is not yet in the table,
        // we can return a "tombstone" which is also empty.
        return tombstone != NULL ? tombstone : entry;
      } else {
        // we found a tombstone
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (entry->key == key) {
      // equality works because strings are INTERNED,
      // eg: two strings representing the same chars are pointed
      // by the same pointer.
      return entry;
    }
    // probe next entry, 'cause the one we found doesn't match our key.
    index = (index + 1) % capacity;
  }
}

static void adjustCapacity(Table *table, int capacity) {
  Entry *entries = ALLOCATE(Entry, capacity);
  // basically memset(0)
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  // re-build the whole HashMap
  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry *src = &table->entries[i];
    if (src->key == NULL)
      continue;

    Entry *dest = findEntry(entries, capacity, src->key);
    dest->key = src->key;
    dest->value = src->value;
    table->count++;
  }

  // update table
  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool tableGet(Table *table, ObjString *key, Value *value) {
  if (table->count == 0)
    return false;
  Entry *entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return false;
  }
  *value = entry->value;
  return true;
}

bool tableSet(Table *table, ObjString *key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }

  // findEntry() directly returns the bucket pointer
  Entry *entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = entry->key == NULL;
  if (isNewKey && IS_NIL(entry->value)) // IS_NIL() checks if its a tombstone
    table->count++;
  entry->key = key;
  entry->value = value;
  return isNewKey;
}
bool tableDelete(Table *table, ObjString *key) {
  if (table->count == 0)
    return false;
  Entry *del_entry = findEntry(table->entries, table->capacity, key);
  if (del_entry->key == NULL)
    return false;
  // Empty the entry bucket by placing a special
  // maker in it: a tombstone.
  // tombones have a NULL key and a True value.
  del_entry->key = NULL;
  del_entry->value = BOOL_VAL(true);
  // DO NOT table->count --; otherwise we might end up with an "empty"
  // Table, full of tombstone
  return true;
}

/**
 * Copy hashmap values from `from` into `to`.
 */
void tableAddAll(Table *from, Table *to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry *src = &from->entries[i];
    if (src->key == NULL)
      continue;
    tableSet(to, src->key, src->value);
  }
}

ObjString *tableFindString(Table *table, const char *chars, int length,
                           uint32_t hash) {
  if (table->count == 0)
    return NULL;

  uint32_t index = hash % table->capacity;
  Entry *tombstone = NULL;
  for (;;) {
    Entry *entry = &table->entries[index];
    if (entry->key == NULL) {
      // stop if we find an empty non-tombstone entry
      if (IS_NIL(entry->value))
        return NULL;

    } else if ( // full comparaison
        entry->key->length == length && entry->key->hash == hash &&
        memcmp(entry->key->chars, chars, length) == 0) {
      return entry->key;
    }
    // probe next entry, 'cause the one we found doesn't match our key.
    index = (index + 1) % table->capacity;
  }
}

// remove unreachable (marked "white") entries from the table
void tableRemoveWhite(Table *table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->obj.isMarked) {
      tableDelete(table, entry->key);
    }
  }
}

// mark both key and values of the Table
void markTable(Table *table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    markObject((Obj *)entry->key);
    markValue(entry->value);
  }
}
