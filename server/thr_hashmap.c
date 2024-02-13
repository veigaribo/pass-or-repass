#include "thr_hashmap.h"

#include <stdlib.h>

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

void thr_hashmap_init(struct thr_hashmap *h) {
  // pthread_mutex_lock(&global_mutex);

  hashmap_init(&h->data);
  pthread_mutex_init(&h->mutex, NULL);
  h->destroyed = false;

  // pthread_mutex_unlock(&global_mutex);
}

void *thr_hashmap_get(struct thr_hashmap *h, char *key, size_t key_length) {
  void *result;

  pthread_mutex_lock(&h->mutex);

  result = hashmap_get(&h->data, key, key_length);

  pthread_mutex_unlock(&h->mutex);

  return result;
}

void thr_hashmap_put(struct thr_hashmap *h, char *key, size_t key_length,
                     void *value) {
  pthread_mutex_lock(&h->mutex);

  hashmap_put(&h->data, key, key_length, value);

  pthread_mutex_unlock(&h->mutex);
}

void *thr_hashmap_remove(struct thr_hashmap *h, char *key, size_t key_length) {
  void *result;

  pthread_mutex_lock(&h->mutex);

  result = hashmap_remove(&h->data, key, key_length);

  pthread_mutex_unlock(&h->mutex);

  return result;
}

void thr_hashmap_free(struct thr_hashmap *h, bool free_values) {
  pthread_mutex_lock(&global_mutex);

  if (h->destroyed)
    return;

  h->destroyed = true;

  pthread_mutex_destroy(&h->mutex);
  hashmap_free(&h->data, free_values);

  pthread_mutex_unlock(&global_mutex);
}
