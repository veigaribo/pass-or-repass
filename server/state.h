#ifndef H_PASS_OR_REPASS_STATE
#define H_PASS_OR_REPASS_STATE

#include "client.h"
#include "thr_hashmap.h"
#include "thr_vector.h"
#include <pthread.h>

struct state {
  // TODO: Use only this mutex for synchronization and get rid of the
  // others
  struct thr_vector clients;
  struct thr_vector mm_queue;
  struct thr_vector ready_checks;
  struct thr_hashmap games;

  pthread_mutex_t mutex;
};

struct client;

void state_init(struct state *state);

void state_push_client(struct client *client);
struct client *state_get_client(struct state *state, int socket);
void state_free(struct state *state);

#endif
