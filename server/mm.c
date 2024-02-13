#include "mm.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"
#include "debug.h"
#include "event_loop.h"
#include "net_sender.h"
#include "ready_check.h"
#include "thr_vector.h"

struct mm_params {
  struct state *state;
};

struct mm_params mm_params;
pthread_t match_maker;

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

void make_match(struct client *c1, struct client *c2) {
  ready_check_add(c1, c2);
}

void *mm_do_loop(void *params) {
  struct mm_params *cparams = params;
  struct state *state = cparams->state;
  struct thr_vector *mm_queue = &state->mm_queue;

  while (true) {
    pthread_mutex_lock(&global_mutex);
    Debug(MatchMaking, "Match maker trying to match\n");

    if (mm_queue->data.length >= 2) {
      // I should be the only one taking things from the
      // front so it's alright
      struct client *c1 = thr_vector_pop(mm_queue, 0);
      struct client *c2 = thr_vector_pop(mm_queue, 0);

      Debug(MatchMaking, "Match! %d, %d\n", c1->socket, c2->socket);
      make_match(c1, c2);

      Debug(MatchMaking, "MM queue length now: %d\n", mm_queue->data.length);
    }

    pthread_mutex_unlock(&global_mutex);
    sleep(MM_POLLING_INTERVAL_S);
  }
}

void mm_loop_init(struct state *state) {
  mm_params.state = state;
  pthread_create(&match_maker, NULL, mm_do_loop, &mm_params);
}

void mm_client_join(struct client *client) {
  pthread_mutex_lock(&global_mutex);

  struct state *state = client->out_state;
  struct thr_vector *mm_queue = &state->mm_queue;
  int socket = client->socket;

  for (size_t i = 0; i < mm_queue->data.length; ++i) {
    struct client *c2 = mm_queue->data.data[i];
    Debug(MatchMaking, "Current MM queue [%d]: Client %d\n", i, c2->socket);

    if (c2->socket == socket) {
      send_deny_abort(client, NULL);
      goto end;
    }
  }

  thr_vector_push(mm_queue, client);

end:
  pthread_mutex_unlock(&global_mutex);
}

struct client *mm_try_client_quit(struct client *client) {
  pthread_mutex_lock(&global_mutex);

  struct state *state = client->out_state;
  struct thr_vector *mm_queue = &state->mm_queue;
  int socket = client->socket;

  struct client *removed = thr_vector_rm_by_addr(mm_queue, client);
  pthread_mutex_unlock(&global_mutex);

  return removed;
}

void mm_client_quit(struct client *client) {
  struct client *removed = mm_try_client_quit(client);

  if (removed == NULL) {
    send_deny_abort(client, NULL);
    return;
  }
}
