#ifndef H_PASS_OR_REPASS_HASHMAP
#define H_PASS_OR_REPASS_HASHMAP

#include <stdbool.h>

#include "list.h"

#define HASHMAP_BUCKETS 63

struct hashmap_entry {
  char *key;
  void *value;
};

struct hashmap {
  struct list buckets[HASHMAP_BUCKETS];
};

int hash(char *str);
void hashmap_init(struct hashmap *h);
void *hashmap_get(struct hashmap *h, char *key);
void hashmap_put(struct hashmap *h, char *key, int key_length, void *value);
void hashmap_free(struct hashmap *h, bool free_values);

#endif
