#ifndef H_PASS_OR_REPASS_READY_CHECK
#define H_PASS_OR_REPASS_READY_CHECK

#include "state.h"
#include <pthread.h>

struct ready_check {
  struct client *c1;
  struct client *c2;

  bool c1_accepted;
  bool c2_accepted;

  pthread_mutex_t mutex;
  bool destroyed;
};

struct state;

// High level:
struct ready_check *ready_check_add(struct client *c1, struct client *c2);

void ready_check_accept(struct client *client);
void ready_check_abort(struct client *client);

// Low level:
void ready_check_free(struct ready_check *ready_check);

#endif
