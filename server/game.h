#ifndef H_PASS_OR_REPASS_GAME
#define H_PASS_OR_REPASS_GAME

#include "id.h"
#include "question.h"
#include "ready_check.h"
#include <pthread.h>
#include <stdbool.h>

struct player {
  struct client *client;
  struct game *game;

  // 1 or 2
  char player_id;
  int score;
};

struct game {
  char id[ID_STR_LENGTH];
  struct player player_1;
  struct player player_2;

  // Sometimes we need to do something only after we are sure data
  // has been succesfully sent to the client sockets. This is used
  // to ensure the data has been sent to both players, not just one
  size_t players_net_send_count;

  int current_player_id;

  size_t question_count;
  size_t used_questions[6];
  struct question *current_question;

  int current_pass_count;

  pthread_mutex_t mutex;
  bool destroyed;
};

struct state;
struct ready_check;

// High level
struct game *maybe_game_start(struct ready_check *check);
void game_abort(struct client *client);
void game_answer(struct client *client, enum question_answer answer);
void game_pass(struct client *client);

#endif
