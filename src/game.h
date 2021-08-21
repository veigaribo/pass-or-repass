#ifndef H_PASS_OR_REPASS_GAME
#define H_PASS_OR_REPASS_GAME

#include "id.h"
#include "player.h"

struct game {
  char id[ID_STR_LENGTH];
  struct player *player_1;
  struct player *player_2;

  int question_count;
  int used_questions[6];
  int current_player_index;
  struct question *current_question;
  int current_pass_count;
  bool just_passed;

  int players_needing_to_ack_score;
};

#endif
