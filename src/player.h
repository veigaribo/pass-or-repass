#ifndef H_PASS_OR_REPASS_PLAYER
#define H_PASS_OR_REPASS_PLAYER

#include "client.h"
#include "game.h"

struct player {
  struct client *client;
  struct game *game;

  // 1 or 2
  char player_id;
  int score;
};

#endif
