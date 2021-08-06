#ifndef H_PASS_OR_REPASS_STATE
#define H_PASS_OR_REPASS_STATE

#include "client.h"
#include "thr_vector.h"

struct state {
  struct thr_vector *clients;
  struct thr_vector *mm_queue;
  struct thr_vector *ready_checks;
  struct thr_hashmap *games;
};

struct client *get_client(struct state *state, int socket);
void free_state(struct state *state);

#endif
