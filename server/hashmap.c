#include "hashmap.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// http://www.cse.yorku.ca/~oz/hash.html
int hash(char *str, size_t count) {
  unsigned int hash = 5381;

  /* hash * 33 + c */
  for (size_t i = 0; i < count; ++i) {
    int c = str[i];
    hash = ((hash << 5) + hash) + c;
  }

  return hash % HASHMAP_BUCKETS;
}

void hashmap_init(struct hashmap *h) { memset(h->buckets, 0, HASHMAP_BUCKETS); }

void *hashmap_get(struct hashmap *h, char *key, size_t key_length) {
  int hashed = hash(key, key_length);
  struct list *bucket = h->buckets[hashed];

  if (bucket == NULL) {
    return NULL;
  }

  struct hashmap_entry *entry = bucket->data;

  while (entry != NULL) {
    bool found = strncmp(key, entry->key, key_length) == 0;

    if (found) {
      return entry->value;
    }

    bucket = bucket->next;
    entry = bucket->data;
  }

  return NULL;
}

void hashmap_put(struct hashmap *h, char *key, size_t key_length, void *value) {
  char *heap_key = malloc(sizeof(char) * key_length);
  memcpy(heap_key, key, key_length);

  struct hashmap_entry *entry = malloc(sizeof(struct hashmap_entry));
  entry->value = value;
  entry->key = heap_key;

  struct list *list = malloc(sizeof(struct list));
  list->data = entry;

  int hashed = hash(key, key_length);
  struct list *bucket = h->buckets[hashed];

  if (bucket == NULL) {
    h->buckets[hashed] = list;
    return;
  }

  while (bucket->next) {
    bucket = bucket->next;
  }

  bucket->next = list;
  list->previous = bucket;
}

void *hashmap_remove(struct hashmap *h, char *key, size_t key_length) {
  int hashed = hash(key, key_length);
  struct list *bucket = h->buckets[hashed];

  if (bucket == NULL) {
    return NULL;
  }

  struct list *previous = NULL;
  struct list *next = bucket->next;

  struct hashmap_entry *entry = bucket->data;

  while (entry != NULL) {
    if (strncmp(key, entry->key, key_length) == 0) {

      if (previous == NULL && next == NULL) {
        free(bucket);
        h->buckets[hashed] = NULL;
      } else {
        if (previous != NULL) {
          previous->next = next;
        }

        if (next != NULL) {
          next->previous = previous;
        }
      }

      void *value = entry->value;

      free(entry->key);
      free(entry);

      return value;
    }

    previous = bucket;
    bucket = previous->next;
    next = bucket->next;

    entry = bucket->data;
  }

  return NULL;
}

void hashmap_free(struct hashmap *h, bool free_values) {
  struct list *bucket, *next;
  for (size_t i = 0; i < HASHMAP_BUCKETS; i++) {
    bucket = h->buckets[i];

    if (bucket == NULL) {
      continue;
    }

    struct hashmap_entry *entry = bucket->data;

    while (entry != NULL) {
      next = bucket->next;
      free(entry->key);

      if (free_values) {
        free(entry->value);
      }

      free(entry);
      free(bucket);

      bucket = next;
      struct hashmap_entry *entry = bucket->data;
    }
  }
}
