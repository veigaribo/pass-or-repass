#include "ready_check.h"
#include "net_sender.h"
#include "thr_vector.h"
#include <stdlib.h>

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

static void ready_check_init(struct ready_check *ready_check, struct client *c1,
                             struct client *c2) {
  ready_check->c1 = c1;
  ready_check->c2 = c2;
  ready_check->c1_accepted = false;
  ready_check->c2_accepted = false;

  pthread_mutex_init(&ready_check->mutex, NULL);
  ready_check->destroyed = false;

  c1->ready_check = ready_check;
  c2->ready_check = ready_check;
}

// Thread unsafe!!
static void ready_check_nothr_free(struct ready_check *ready_check) {
  pthread_mutex_destroy(&ready_check->mutex);
  ready_check->c1->ready_check = NULL;
  ready_check->c2->ready_check = NULL;

  ready_check->destroyed = true;
}

void ready_check_free(struct ready_check *ready_check) {
  pthread_mutex_lock(&global_mutex);

  if (ready_check->destroyed) {
    goto end;
  }

  ready_check_nothr_free(ready_check);

end:
  pthread_mutex_unlock(&global_mutex);
}

struct ready_check *ready_check_add(struct client *c1, struct client *c2) {
  // No need for mutex paranoia yet
  struct ready_check *ready_check = malloc(sizeof(struct ready_check));
  ready_check_init(ready_check, c1, c2);

  // Assume both are the same
  struct state *state = c1->out_state;

  struct thr_vector *ready_checks = &state->ready_checks;
  thr_vector_push(ready_checks, ready_check);

  send_game_found(c1, NULL);
  send_game_found(c2, NULL);

  return ready_check;
}

void ready_check_accept(struct client *client) {
  struct ready_check *check = client->ready_check;
  pthread_mutex_lock(&check->mutex);

  if (check == NULL) {
    goto end;
  }

  if (client == check->c1) {
    check->c1_accepted = true;
  } else if (client == check->c2) {
    check->c2_accepted = true;
  }

  maybe_game_start(check);

end:
  pthread_mutex_unlock(&check->mutex);
}

void ready_check_abort(struct client *client) {
  pthread_mutex_lock(&global_mutex);

  struct ready_check *check = client->ready_check;

  if (check->destroyed) {
    goto end;
  }

  struct client *c1 = check->c1;
  struct client *c2 = check->c2;

  struct state *state = client->out_state;

  struct thr_vector *checks = &state->ready_checks;
  thr_vector_rm_by_addr(checks, check);
  ready_check_nothr_free(check);

  free(check);

  if (c1->socket != -1) {
    send_game_aborted(c1, NULL);
  }

  if (c2->socket != -1) {
    send_game_aborted(c2, NULL);
  }

end:
  pthread_mutex_unlock(&global_mutex);
}
