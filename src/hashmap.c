#include "hashmap.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// http://www.cse.yorku.ca/~oz/hash.html
int hash(char *str) {
  unsigned int hash = 5381;
  int c;

  /* hash * 33 + c */
  while ((c = *str++)) hash = ((hash << 5) + hash) + c;

  return hash % HASHMAP_BUCKETS;
}

void hashmap_init(struct hashmap *h) { memset(h->buckets, 0, HASHMAP_BUCKETS); }

void *hashmap_get(struct hashmap *h, char *key) {
  int hashed = hash(key);

  struct list *bucket = &h->buckets[hashed];

  struct hashmap_entry *entry;
  bool found;

  while (bucket != NULL) {
    entry = bucket->data;
    found = strcmp(key, entry->key) == 0;

    if (found) {
      return entry->value;
    }

    bucket = bucket->next;
  }

  return NULL;
}

void hashmap_put(struct hashmap *h, char *key, int key_length, void *value) {
  char *heap_key = malloc(sizeof(char *) * key_length);
  memcpy(heap_key, key, key_length);

  struct hashmap_entry *entry = malloc(sizeof(struct hashmap_entry));
  entry->value = value;
  entry->key = heap_key;

  struct list *list = malloc(sizeof(struct list));
  list->data = value;

  int hashed = hash(key);
  struct list *bucket = &h->buckets[hashed];

  if (bucket == NULL) {
    bucket->data = entry;
  }

  while (bucket->next) {
    bucket = bucket->next;
  }

  bucket->next = list;
  list->previous = bucket;
}

void hashmap_free(struct hashmap *h, bool free_values) {
  struct list *bucket, *next;
  for (int i = 0; i < HASHMAP_BUCKETS; i++) {
    bucket = &h->buckets[i];

    while (bucket != NULL) {
      next = bucket->next;
      struct hashmap_entry *entry = bucket->data;

      free(entry->key);

      if (free_values) {
        free(entry->value);
      }

      free(entry);
      free(bucket);

      bucket = next;
    }
  }
}
