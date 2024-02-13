#include "client.h"
#include "net_parser.h"
#include <stdlib.h>
#include <string.h>

void client_init(struct client *client, struct state *state, int socket) {
  client->socket = socket;
  client->out_state = state;
  client->player = NULL;
  client->ready_check = NULL;

  client->net_parser = malloc(sizeof(struct net_parser));
  net_parser_init(client->net_parser, socket);

  client->name = "Anon";
  client->name_len = strlen("Anon");
}

void client_free(struct client *client) { free(client->net_parser); }
