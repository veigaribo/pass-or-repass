#include "player_queue.h"

#include <stdlib.h>

#include "thr_vector.h"

void on_client_connected(struct client_connected_params *data) {
  struct client *client = malloc(sizeof(struct client));
  client->socket = data->socket;

  thr_vector_push(data->clients, client);
}
