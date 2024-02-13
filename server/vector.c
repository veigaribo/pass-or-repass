#include "vector.h"

#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

// Vector ata will always live on the heap
void vector_init(struct vector *dest, int length, int capacity) {
  void *data = malloc((sizeof(void *)) * capacity);

  dest->data = data;
  dest->length = length;
  dest->capacity = capacity;
}

void vector_grow(struct vector *original) {
  int old_capacity = original->capacity;
  int new_capacity = old_capacity + (old_capacity >> 1);

  void *data = realloc(original->data, (sizeof(void *)) * new_capacity);

  original->data = data;
  original->capacity = new_capacity;
}

void vector_fit(struct vector *original, int length) {
  while (original->capacity < length) {
    vector_grow(original);
  }
}

void vector_push(struct vector *vec, void *data) {
  int new_length = vec->length + 1;

  if (new_length > vec->capacity) {
    vector_grow(vec);
  }

  vec->data[vec->length] = data;
  vec->length = new_length;
}

void *vector_rm_by_addr(struct vector *original, void *addr) {
  void *current, *result = NULL;
  int length = original->length;

  for (int i = 0, new_i = 0; i < length; i++) {
    current = original->data[i];

    // The last index will be dirty but we don't care
    if (current == addr) {
      result = current;
      --original->length;
    } else {
      original->data[new_i] = original->data[i];
      ++new_i;
    }
  }

  return result;
}

void *vector_pop(struct vector *original, int index) {
  void *current, *result = NULL;
  Debug(Vector, "Popping index %d\n", index);
  Debug(Vector, "Has length %d\n", original->length);
  int length = original->length;

  for (int i = 0, new_i = 0; i < length; i++) {
    Debug(Vector, "Iterating to pop | i = %d, new_i = %d\n", i, new_i);
    current = original->data[i];

    // The last index will be dirty but we don't care
    if (i == index) {
      Debug(Vector, "Pop found\n");
      result = current;
      --original->length;
    } else {
      Debug(Vector, "Not popping\n");
      original->data[new_i] = original->data[i];
      ++new_i;
    }
  }

  return result;
}

// !
void vector_free(struct vector *vec) { free(vec->data); }
