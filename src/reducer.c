#include "reducer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "id.h"
#include "protocol.h"
#include "question.h"
#include "ready_check.h"
#include "thr_hashmap.h"

void sendall(int socket, char *buffer, int length) {
  unsigned char message = ACK;
  int total_sent = 0;

  while (total_sent < length) {
    total_sent += send(socket, buffer, length, 0);
  }
}

void send_ack(int socket) {
  char message = ACK;
  sendall(socket, &message, 1);
}

void send_deny(int socket) {
  printf("DENYING!!!!!!!!!\n");
  char message = DENY;
  sendall(socket, &message, 1);
}

void join_queue(struct handle_params *params) {
  struct state *state = params->state;
  struct game_event event = params->event;

  struct thr_vector *mm_queue = state->mm_queue;

  struct client *client = get_client(state, event.client_socket);
  int socket = client->socket;

  for (int i = 0; i < mm_queue->data->length; i++) {
    struct client *c2 = mm_queue->data->data[i];

    printf("Current MM queue [%d]: Client %d\n", i, c2->socket);

    if (c2->socket == event.client_socket) {
      send_deny(socket);
      return;
    }
  }

  thr_vector_push(mm_queue, client);

  send_ack(socket);

  free(params);
}

void quit_queue(struct handle_params *params) {
  struct state *state = params->state;
  struct game_event event = params->event;

  struct thr_vector *mm_queue = state->mm_queue;

  struct client *client = get_client(state, event.client_socket);
  int socket = client->socket;

  struct client *removed = thr_vector_rm_by_addr(mm_queue, client);

  if (removed == NULL) {
    send_deny(socket);
    return;
  }

  send_ack(socket);

  free(params);
}

void make_ready_check(struct reducer_params params) {
  struct handle_params *handle_params = params.handle_params;
  struct state *state = handle_params->state;

  struct make_ready_check_extra *extra = params.extra;

  struct thr_vector *ready_checks = state->ready_checks;

  struct ready_check *ready_check = malloc(sizeof(struct ready_check));
  ready_check->c1 = extra->c1;
  ready_check->c2 = extra->c2;
  ready_check->accepteds = 0;

  extra->c1->ready_check = ready_check;
  extra->c2->ready_check = ready_check;

  thr_vector_push(ready_checks, ready_check);

  // Sending the events serially like that is bad but I've no time
  char message = GAME_FOUND;
  sendall(extra->c1->socket, &message, 1);
  sendall(extra->c2->socket, &message, 1);

  free(extra);
  free(handle_params);
}

// Should use extra but this is easier
void ask_question(struct reducer_params params, struct game *game) {
  struct handle_params *handle_params = params.handle_params;
  struct state *state = handle_params->state;
  struct game_event event = handle_params->event;

  int player_index = game->current_player_index;

  struct player *p1 = game->player_1;
  struct player *p2 = game->player_2;

  struct client *c1 = p1->client;
  int socket1 = c1->socket;

  struct client *c2 = p2->client;
  int socket2 = c2->socket;

  int *used_questions = game->used_questions;
  struct question *question;
  bool is_used;

  if (game->just_passed) {
    question = game->current_question;
    game->just_passed = false;
  } else {
    // Naive way to not repeat questions
    while (true) {
      is_used = false;
      question = get_question();

      for (int i = 0; i < 5; i++) {
        int used_question = used_questions[i];

        if (used_question != 0 && used_question == question->id) {
          is_used = true;
          break;
        }
      }

      if (!is_used) {
        used_questions[game->question_count] = question->id;
        break;
      }
    }
  }

  game->current_question = question;

  char ask_event = player_index == 0 ? ASK_TO_01 : ASK_TO_02;

  printf("Sending question to c1\n");

  sendall(socket1, &ask_event, 1);
  sendall(socket1, question->text, question->length);

  printf("Sending question to c2\n");

  sendall(socket2, &ask_event, 1);
  sendall(socket2, question->text, question->length);

  free(handle_params);
}

void ask_next_question(struct reducer_params params) {
  struct handle_params *handle_params = params.handle_params;
  struct state *state = handle_params->state;
  struct game_event event = handle_params->event;

  struct client *client = get_client(state, event.client_socket);
  struct player *player = client->player;

  if (player == NULL) {
    printf("No player\n");
    send_deny(client->socket);
    return;
  }

  struct game *game = player->game;

  if (game == NULL) {
    printf("No game\n");
    send_deny(client->socket);
    return;
  }

  --game->players_needing_to_ack_score;

  printf("Got store ack %d\n", game->players_needing_to_ack_score);

  if (game->players_needing_to_ack_score == 0) {
    ask_question(params, game);
  } else {
    free(handle_params);
  }
}

void maybe_make_game(struct reducer_params params) {
  struct handle_params *handle_params = params.handle_params;
  struct state *state = handle_params->state;
  struct game_event event = handle_params->event;

  struct client *client = get_client(state, event.client_socket);
  int socket = client->socket;

  struct ready_check *ready_check = client->ready_check;

  if (ready_check == NULL) {
    send_deny(socket);
    return;
  }

  ++ready_check->accepteds;

  if (ready_check->accepteds < 2) {
    return;
  }

  struct client *c1 = ready_check->c1;
  struct client *c2 = ready_check->c2;

  struct thr_hashmap *games = state->games;

  long nmatch_id = id_get();
  char match_id[64];

  id_get_str(match_id);

  printf("Generated ID %s\n", match_id);

  struct player *p1 = malloc(sizeof(struct player));
  p1->client = c1;
  p1->player_id = 1;

  struct player *p2 = malloc(sizeof(struct player));
  p2->client = c2;
  p2->player_id = 2;

  struct game *game = malloc(sizeof(struct game));
  memcpy(game->id, match_id, ID_STR_LENGTH);
  game->player_1 = p1;
  game->player_2 = p2;
  game->question_count = 0;
  memset(game->used_questions, 0, 5);
  game->current_player_index = 0;
  game->current_question = NULL;
  game->current_pass_count = 0;
  game->players_needing_to_ack_score = 0;
  game->just_passed = false;

  p1->game = game;
  p2->game = game;

  thr_hashmap_put(games, match_id, ID_STR_LENGTH, game);

  c1->ready_check = NULL;
  c2->ready_check = NULL;
  c1->player = p1;
  c2->player = p2;

  // Type, match ID, null, player ID, null
  const int event_length = 1 + ID_STR_LENGTH + 1 + 1 + 1;

  // Sending the events serially like that is bad but I've no time

  char event1[event_length];
  memset(event1, 0, event_length);
  event1[0] = JOINED_GAME;
  memcpy(&event1[1], match_id, ID_STR_LENGTH);
  memcpy(&event1[event_length - 2], &p1->player_id, 1);

  sendall(c1->socket, event1, event_length);

  char event2[event_length];
  memset(event2, 0, event_length);
  event2[0] = JOINED_GAME;
  memcpy(&event2[1], match_id, ID_STR_LENGTH);
  memcpy(&event2[event_length - 2], &p2->player_id, 1);

  sendall(c2->socket, event2, event_length);

  c1->needs_to_ack_score = true;
  c2->needs_to_ack_score = true;
  game->players_needing_to_ack_score = 2;

  free(handle_params);
}

void handle_ack(struct reducer_params params) {
  struct handle_params *handle_params = params.handle_params;
  struct state *state = handle_params->state;
  struct game_event event = handle_params->event;

  struct client *client = get_client(state, event.client_socket);

  if (client->ready_check != NULL) {
    // Accepted game
    maybe_make_game(params);
    return;
  } else if (client->needs_to_ack_score) {
    client->needs_to_ack_score = false;
    ask_next_question(params);
    return;
  }

  // Ignore by default
  free(handle_params);
}

void answer_question(struct reducer_params params,
                     enum question_answer answer) {
  struct handle_params *handle_params = params.handle_params;
  struct state *state = handle_params->state;
  struct game_event event = handle_params->event;

  struct client *client = get_client(state, event.client_socket);
  struct player *player = client->player;

  if (player == NULL) {
    printf("No player\n");
    send_deny(client->socket);
    return;
  }

  struct game *game = player->game;

  if (game == NULL) {
    printf("No game\n");
    send_deny(client->socket);
    return;
  }

  int player_index = game->current_player_index;

  // Player ID is {1, 2}, player index is {0, 1}
  if (player->player_id != player_index + 1) {
    printf("Wrong player: %d | %d\n", player->player_id, player_index + 1);

    send_deny(client->socket);
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

  char score_message[3];

  char score_event = player_index == 0 ? SCORE_01 : SCORE_02;

  memset(score_message, 0, 3);
  score_message[0] = score_event;

  int score_to_send = player->score + 1;
  memcpy(&score_message[1], &score_to_send, 1);

  struct client *c1 = game->player_1->client;
  struct client *c2 = game->player_2->client;

  c1->needs_to_ack_score = true;
  c2->needs_to_ack_score = true;
  game->players_needing_to_ack_score = 2;

  printf("B CURRENT PLAYER INDEX %d\n --------", game->current_player_index);
  game->current_player_index = !player_index;
  printf("A CURRENT PLAYER INDEX %d\n --------", game->current_player_index);
  game->current_pass_count = 0;

  sendall(c1->socket, score_message, 3);
  sendall(c2->socket, score_message, 3);

  printf("Question count %d\n", game->question_count);

  if (game->question_count == 5) {
    char message = GAME_END;
    sendall(c1->socket, &message, 1);
    sendall(c2->socket, &message, 1);
    return;
  }

  ++game->question_count;

  // Awaits ack
  free(handle_params);
}

void pass_question(struct reducer_params params) {
  struct handle_params *handle_params = params.handle_params;
  struct state *state = handle_params->state;
  struct game_event event = handle_params->event;

  struct client *client = get_client(state, event.client_socket);
  struct player *player = client->player;

  if (player == NULL) {
    printf("No player\n");
    send_deny(client->socket);
    return;
  }

  struct game *game = player->game;

  if (game == NULL) {
    printf("No game\n");
    send_deny(client->socket);
    return;
  }

  int player_index = game->current_player_index;

  // Player ID is {1, 2}, player index is {0, 1}
  if (player->player_id != player_index + 1) {
    printf("Wrong player: %d | %d\n", player->player_id, player_index + 1);
    send_deny(client->socket);
    return;
  }

  if (game->current_pass_count >= 2) {
    printf("Max passes");
    send_deny(client->socket);
    return;
  }

  char score_message[3];

  char score_event = player_index == 0 ? SCORE_01 : SCORE_02;

  memset(score_message, 0, 3);
  score_message[0] = score_event;

  int score_to_send = player->score + 1;
  memcpy(&score_message[1], &score_to_send, 1);

  struct client *c1 = game->player_1->client;
  struct client *c2 = game->player_2->client;

  c1->needs_to_ack_score = true;
  c2->needs_to_ack_score = true;
  game->players_needing_to_ack_score = 2;

  printf("D CURRENT PLAYER INDEX %d\n --------", game->current_player_index);
  game->current_player_index = (player_index + 1) % 2;
  printf("C CURRENT PLAYER INDEX %d\n --------", game->current_player_index);
  ++game->current_pass_count;
  game->just_passed = true;

  sendall(c1->socket, score_message, 3);
  sendall(c2->socket, score_message, 3);

  printf("Question count %d\n", game->question_count);
  free(handle_params);
}

void disconnect_client(struct reducer_params params) {
  struct handle_params *handle_params = params.handle_params;
  struct state *state = handle_params->state;
  struct game_event event = handle_params->event;

  struct client *client = get_client(state, event.client_socket);
  struct player *player = client->player;

  if (player != NULL) {
    struct game *game = player->game;

    if (player->player_id == 1) {
      game->player_1 = NULL;
    } else {
      game->player_2 = NULL;
    }

    free(player);
  }

  thr_vector_rm_by_addr(state->mm_queue, client);
  thr_vector_rm_by_addr(state->clients, client);

  free(handle_params);
  close(client->socket);
  free(client);
}
