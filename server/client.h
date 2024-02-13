#ifndef H_PASS_OR_REPASS_CLIENT
#define H_PASS_OR_REPASS_CLIENT

#include <stdbool.h>
#include <stddef.h>

#include "game.h"
#include "net_parser.h"
#include "state.h"

struct client {
  int socket;

  // TODO: This could be embedded but it seems header recursion is
  // preventing that somehow (?)
  struct net_parser *net_parser;

  bool busy;

  struct state *out_state;

  struct player *player;
  struct ready_check *ready_check;

  char *name;
  size_t name_len;
};

void client_init(struct client *client, struct state *state, int socket);
void client_free(struct client *client);

#endif
