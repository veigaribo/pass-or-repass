#ifndef H_PASS_OR_REPASS_GAME
#define H_PASS_OR_REPASS_GAME

#include "player.h"

struct game {
  char id[64];
  struct player *player_1;
  struct player *player_2;

  int question_count;
  // There will be 6 questions; the last one
  // we don't need to store
  int used_questions[5];
  int current_player_index;
  struct question *current_question;
  int current_pass_count;
  bool just_passed;

  int players_needing_to_ack_score;
};

#endif
