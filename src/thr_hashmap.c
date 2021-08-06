#include "thr_hashmap.h"

#include <stdlib.h>

void thr_hashmap_init(struct thr_hashmap *h) {
  struct hashmap *data = malloc(sizeof(struct hashmap));
  hashmap_init(data);

  pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);

  h->data = data;
  h->mutex = mutex;
}

void *thr_hashmap_get(struct thr_hashmap *h, char *key) {
  void *result;

  pthread_mutex_lock(h->mutex);

  result = hashmap_get(h->data, key);

  pthread_mutex_unlock(h->mutex);

  return result;
}

void thr_hashmap_put(struct thr_hashmap *h, char *key, int key_length,
                     void *value) {
  pthread_mutex_lock(h->mutex);

  hashmap_put(h->data, key, key_length, value);

  pthread_mutex_unlock(h->mutex);
}

void thr_hashmap_free(struct thr_hashmap *h, bool free_values) {
  pthread_mutex_lock(h->mutex);

  hashmap_free(h->data, free_values);

  pthread_mutex_unlock(h->mutex);

  pthread_mutex_destroy(h->mutex);

  free(h->data);
  free(h->mutex);
}
