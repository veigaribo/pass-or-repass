#ifndef H_PASS_OR_REPASS_THR_HASHMAP
#define H_PASS_OR_REPASS_THR_HASHMAP

#include <pthread.h>
#include <stdbool.h>

#include "hashmap.h"

struct thr_hashmap {
  struct hashmap *data;
  pthread_mutex_t *mutex;
};

void thr_hashmap_init(struct thr_hashmap *h);
void *thr_hashmap_get(struct thr_hashmap *h, char *key);
void thr_hashmap_put(struct thr_hashmap *h, char *key, int key_length,
                     void *value);
void thr_hashmap_free(struct thr_hashmap *h, bool free_values);

#endif
