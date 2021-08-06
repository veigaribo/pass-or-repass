#ifndef H_PASS_OR_REPASS_CLIENT
#define H_PASS_OR_REPASS_CLIENT

#include <stdbool.h>

#include "player.h"

struct client {
  int socket;
  struct player *player;
  struct ready_check *ready_check;

  bool needs_to_ack_score;
};

#endif
