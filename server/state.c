#include "state.h"

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"

void state_init(struct state *state) {
  pthread_mutex_init(&state->mutex, NULL);
  thr_vector_init(&state->clients);
  thr_vector_init(&state->mm_queue);
  thr_vector_init(&state->ready_checks);
  thr_hashmap_init(&state->games);
}

void state_push_client(struct client *client) {
  struct state *state = client->out_state;
  pthread_mutex_lock(&state->mutex);

  thr_vector_push(&state->clients, client);

  pthread_mutex_unlock(&state->mutex);
}

struct client *state_get_client(struct state *state, int socket) {
  pthread_mutex_lock(&state->mutex);

  struct thr_vector *thr_clients = &state->clients;
  struct vector *clients = &thr_clients->data;

  struct client *client = NULL;
  for (size_t i = 0; i < clients->length; i++) {
    client = clients->data[i];

    if (client->socket == socket) {
      goto end;
    }
  }

  client = NULL;

end:
  pthread_mutex_unlock(&state->mutex);
  return client;
}

void state_free(struct state *state) {
  pthread_mutex_destroy(&state->mutex);
  thr_vector_free(&state->clients);
  thr_vector_free(&state->mm_queue);
  thr_vector_free(&state->ready_checks);
  thr_hashmap_free(&state->games, true);
}
