#include "game.h"
#include "debug.h"
#include "id.h"
#include "net_sender.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

static void game_init(struct game *game, char *match_id, struct client *c1,
                      struct client *c2) {
  game->player_1.client = c1;
  game->player_1.player_id = 1;
  game->player_1.score = 0;

  game->player_2.client = c2;
  game->player_2.player_id = 2;
  game->player_1.score = 0;

  game->players_net_send_count = 0;
  game->current_player_id = 1;

  memcpy(game->id, match_id, ID_STR_LENGTH);
  game->question_count = 0;
  memset(game->used_questions, 0, 5);
  game->current_question = NULL;
  game->current_pass_count = 0;

  game->player_1.game = game;
  game->player_2.game = game;

  c1->player = &game->player_1;
  c2->player = &game->player_2;

  pthread_mutex_init(&game->mutex, NULL);
  game->destroyed = false;
}

// Thread unsafe!!
static void game_free(struct game *game) {
  pthread_mutex_destroy(&game->mutex);
  game->player_1.game = NULL;
  game->player_2.game = NULL;

  game->destroyed = true;
}

static void ask(struct client *client) {
  struct game *game = client->player->game;

  int player_index = game->current_player_id;

  struct player *p1 = &game->player_1;
  struct player *p2 = &game->player_2;

  struct client *c1 = p1->client;
  struct client *c2 = p2->client;

  size_t *used_questions = game->used_questions;

  struct question *question =
      get_question(game->used_questions, game->question_count);

  game->current_question = question;
  game->used_questions[game->question_count++] = question->id;

  Debug(Reducer, "Sending question to %s\n", game->id);

  if (player_index == 1) {
    send_ask_to_1(c1, NULL);
    send_ask_to_1(c2, NULL);
  } else {
    send_ask_to_2(c1, NULL);
    send_ask_to_2(c2, NULL);
  }
}

static void joined_game_callback(struct client *client);

struct game *maybe_game_start(struct ready_check *check) {
  // No need for mutex paranoia yet
  if (!(check->c1_accepted && check->c2_accepted)) {
    return NULL;
  }

  struct client *c1 = check->c1;
  struct client *c2 = check->c2;

  char match_id[ID_STR_LENGTH];
  id_get_str(match_id);

  Debug(Game, "Generated ID %s\n", match_id);

  struct game *game = malloc(sizeof(struct game));
  game_init(game, match_id, c1, c2);

  // Assume both are the same
  struct state *state = c1->out_state;

  struct thr_hashmap *games = &state->games;
  thr_hashmap_put(games, match_id, ID_STR_LENGTH, game);

  game->players_net_send_count = 0;
  send_joined_game(c1, joined_game_callback);
  send_joined_game(c2, joined_game_callback);

  struct thr_vector *checks = &state->ready_checks;
  thr_vector_rm_by_addr(checks, check);
  ready_check_free(check);
  free(check);

  return game;
}

void joined_game_callback(struct client *client) {
  struct game *game = client->player->game;
  game->players_net_send_count += 1;

  if (game->players_net_send_count < 2) {
    return;
  }

  ask(client);
}

void game_abort(struct client *client) {
  pthread_mutex_lock(&global_mutex);

  struct player *player = client->player;
  struct game *game = client->player->game;

  if (game->destroyed) {
    goto end;
  }

  struct client *c1 = game->player_1.client;
  struct client *c2 = game->player_2.client;

  // Assume both are the same
  struct state *state = c1->out_state;

  struct thr_hashmap *games = &state->games;
  thr_hashmap_remove(games, game->id, ID_STR_LENGTH);

  game_free(game);
  free(game);

  if (c1->socket != -1) {
    send_game_aborted(c1, NULL);
  }

  if (c2->socket != -1) {
    send_game_aborted(c2, NULL);
  }

end:
  pthread_mutex_unlock(&global_mutex);
}

static void game_answer_callback(struct client *client);

void game_answer(struct client *client, enum question_answer answer) {
  struct player *player = client->player;
  struct game *game = player->game;

  pthread_mutex_unlock(&game->mutex);

  if (game->current_player_id != player->player_id) {
    if (client->socket != -1) {
      send_deny_abort(client, NULL);
    }

    pthread_mutex_unlock(&game->mutex);
    return;
  }

  struct question *question = game->current_question;
  enum question_answer correct_answer = question->correct_answer;

  if (answer == correct_answer) {
    if (game->current_pass_count == 0) {
      player->score += 20;
    } else if (game->current_pass_count == 1) {
      player->score += 10;
    } else if (game->current_pass_count == 2) {
      player->score += 10;
    }
  } else {
    if (game->current_pass_count == 0) {
      player->score -= 10;
    } else if (game->current_pass_count == 1) {
      player->score -= 5;
    } else if (game->current_pass_count == 2) {
      player->score -= 5;
    }
  }

  // Ensure the next callback only happens after both are sent
  game->players_net_send_count = 0;

  if (game->current_player_id == 1) {
    send_score_1(game->player_1.client, game_answer_callback);
    send_score_1(game->player_2.client, game_answer_callback);
  } else {
    send_score_2(game->player_1.client, game_answer_callback);
    send_score_2(game->player_2.client, game_answer_callback);
  }
}

void game_answer_callback(struct client *client) {
  struct game *game = client->player->game;
  game->players_net_send_count += 1;

  if (game->players_net_send_count < 2) {
    return; // Do not unlock. Intentional
  }

  game->current_player_id = game->current_player_id == 1 ? 2 : 1;

  if (game->question_count == 5) {
    send_game_end(game->player_1.client, NULL);
    send_game_end(game->player_2.client, NULL);
  } else {
    ask(client);
  }

  pthread_mutex_unlock(&game->mutex);
}

void game_pass(struct client *client) {
  struct player *player = client->player;
  struct game *game = player->game;

  pthread_mutex_lock(&game->mutex);

  if (game->current_player_id != player->player_id) {
    if (client->socket != -1) {
      send_deny_abort(client, NULL);
    }

    return;
  }

  if (game->current_pass_count >= 2) {
    Debug(Reducer, "Max passes\n");
    send_deny_abort(client, NULL);
    return;
  }

  game->current_player_id = game->current_player_id == 1 ? 2 : 1;
  ++game->current_pass_count;

  if (game->current_player_id == 1) {
    send_pass_to_1(game->player_1.client, NULL);
    send_pass_to_1(game->player_2.client, NULL);
  } else {
    send_pass_to_2(game->player_1.client, NULL);
    send_pass_to_2(game->player_2.client, NULL);
  }

  pthread_mutex_unlock(&game->mutex);
}
