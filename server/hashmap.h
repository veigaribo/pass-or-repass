#ifndef H_PASS_OR_REPASS_HASHMAP
#define H_PASS_OR_REPASS_HASHMAP

#include <stdbool.h>
#include <stddef.h>

#include "list.h"

#define HASHMAP_BUCKETS 63

struct hashmap_entry {
  char *key;
  void *value;
};

struct hashmap {
  struct list *buckets[HASHMAP_BUCKETS];
};

int hash(char *str, size_t count);
void hashmap_init(struct hashmap *h);
void *hashmap_get(struct hashmap *h, char *key, size_t key_length);
void hashmap_put(struct hashmap *h, char *key, size_t key_length, void *value);
void *hashmap_remove(struct hashmap *h, char *key, size_t key_length);
void hashmap_free(struct hashmap *h, bool free_values);

#endif
