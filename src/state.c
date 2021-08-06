#include "state.h"

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"
#include "thr_queue.h"

struct client *get_client(struct state *state, int socket) {
  struct thr_vector *thr_clients = state->clients;
  struct vector clients = *thr_clients->data;

  struct client *client;
  for (int i = 0; i < clients.length; i++) {
    client = clients.data[i];

    if (client->socket == socket) {
      return client;
    }
  }

  return NULL;
}

void free_state(struct state *state) {
  thr_vector_free(state->clients);
  free(state->clients);

  thr_vector_free(state->mm_queue);
  free(state->mm_queue);
}

// Following my logic this should be a reducer :T
