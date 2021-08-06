#include "mm.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"
#include "event_loop.h"
#include "reducer.h"

struct mm_params {
  struct state *state;
};

struct mm_params mm_params;
pthread_t match_maker;

void make_match(struct state *state, struct client *c1, struct client *c2) {
  // Not used
  struct game_event event;

  struct handle_params *handle_params = malloc(sizeof(struct handle_params));
  handle_params->event = event;
  handle_params->state = state;

  struct make_ready_check_extra *extra =
      malloc(sizeof(struct make_ready_check_extra));

  extra->c1 = c1;
  extra->c2 = c2;

  struct reducer_params reducer_params;
  reducer_params.handle_params = handle_params;
  reducer_params.extra = extra;

  make_ready_check(reducer_params);
}

void *mm_do_loop(void *params) {
  struct mm_params *cparams = params;
  struct state *state = cparams->state;
  struct thr_vector *mm_queue = state->mm_queue;

  while (true) {
    printf("Match maker trying to match\n");

    if (mm_queue->data->length >= 2) {
      // I should be the only one taking things from the
      // front so it's alright
      struct client *c1 = thr_vector_pop(mm_queue, 0);
      struct client *c2 = thr_vector_pop(mm_queue, 0);

      printf("Match! %d, %d\n", c1->socket, c2->socket);
      make_match(state, c1, c2);

      printf("MM queue length now: %d\n", mm_queue->data->length);
    }

    sleep(MM_POLLING_INTERVAL_S);
  }
}

void mm_loop_init(struct state *state) {
  mm_params.state = state;

  pthread_create(&match_maker, NULL, mm_do_loop, &mm_params);
}
