#ifndef H_PASS_OR_REPASS_INTERPRETER
#define H_PASS_OR_REPASS_INTERPRETER

#include "state.h"

struct game_event {
  int client_socket;
  unsigned char type;
  char **params;
  int nparams;
};

struct game_event event_get(int socket);
void event_free(struct game_event event);

struct handle_params {
  struct game_event event;
  struct state *state;
};

//  To be called in a message
void handle(void *params);

#endif
