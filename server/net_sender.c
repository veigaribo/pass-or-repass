#include "net_sender.h"
#include "debug.h"
#include "event_loop.h"
#include "message.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

// These will use the event loop

struct message_handler_params {
  struct client *client;
  void (*callback)(struct client *);

  size_t sent_bytes;
};

#define CLIENT(Params) Params->client
#define SOCKET(Params) Params->client->socket
#define PLAYER(Params) Params->client->player
#define GAME(Params) Params->client->player->game

#define LITTLE_ENDIAN_INT(Int)                                                 \
  { Int & 0xff, Int >> 8 & 0xff, Int >> 16 & 0xff, Int >> 24 & 0xff }

static void single_byte_send(struct message_handler_params *params,
                             unsigned char tag) {
  int socket = SOCKET(params);
  unsigned char buffer[] = {tag};

  if (params->sent_bytes == 0) {
    params->sent_bytes += send(socket, buffer, 1, 0);
  }

  if (params->sent_bytes == 1) {
    struct client *client = CLIENT(params);

    if (params->callback)
      params->callback(client);

    free(params);
  }
}

static void send_deny_abort_handler(struct message_handler_params *params) {
  single_byte_send(params, DENY_ABORT);
}
static void send_deny_retry_handler(struct message_handler_params *params) {
  single_byte_send(params, DENY_RETRY);
}
static void send_game_found_handler(struct message_handler_params *params) {
  single_byte_send(params, GAME_FOUND);
}
static void send_pass_to_1_handler(struct message_handler_params *params) {
  single_byte_send(params, PASS_TO_01);
}
static void send_pass_to_2_handler(struct message_handler_params *params) {
  single_byte_send(params, PASS_TO_02);
}
static void send_game_end_handler(struct message_handler_params *params) {
  single_byte_send(params, GAME_END);
}
static void send_game_aborted_handler(struct message_handler_params *params) {
  single_byte_send(params, GAME_ABORTED);
}

void send_joined_game_handler(struct message_handler_params *params) {
  // Size to x means size where x ends
  const int size_to_tag = 1;
  const int size_to_game_id = size_to_tag + 8;
  const int size_to_player_id = size_to_game_id + 1;
  const int final_size = size_to_player_id;

  int socket = SOCKET(params);

  if (params->sent_bytes == 0) {
    unsigned char buffer[] = {JOINED_GAME};
    params->sent_bytes += send(socket, buffer, 1, 0);
  }

  if (params->sent_bytes >= size_to_tag &&
      params->sent_bytes < size_to_game_id) {
    char *buffer = GAME(params)->id;

    size_t offset = params->sent_bytes - size_to_tag;
    size_t left = 8 - offset;

    params->sent_bytes += send(socket, buffer + offset, left, 0);
  }

  if (params->sent_bytes >= size_to_game_id &&
      params->sent_bytes < size_to_player_id) {
    char data = CLIENT(params)->player->player_id;
    params->sent_bytes += send(socket, &data, 1, 0);
  }

  if (params->sent_bytes == final_size) {
    struct client *client = CLIENT(params);

    if (params->callback)
      params->callback(client);

    free(params);
  } else {
    Debug(NetSender, "Partial send (%d/%d) for tag %x, recursing\n",
          params->sent_bytes, final_size, JOINED_GAME);
    struct message *message = message_new(
        params, (void *)send_joined_game_handler, "netsender/joined_game/rec");

    event_loop_add_message(message);
  }
}

static void send_ask_to_x_handler(
    struct message_handler_params *params, unsigned char tag,
    void (*recurse_handler)(struct message_handler_params *)) {
  char *message = GAME(params)->current_question->text;
  size_t message_length = GAME(params)->current_question->length;

  const int size_to_tag = 1;
  const int size_to_length = size_to_tag + 4;
  const int size_to_message = size_to_length + message_length;
  const int final_size = size_to_message;

  int socket = SOCKET(params);

  if (params->sent_bytes == 0) {
    unsigned char buffer[] = {tag};
    params->sent_bytes += send(socket, buffer, 1, 0);
  }

  if (params->sent_bytes >= size_to_tag &&
      params->sent_bytes < size_to_length) {
    unsigned char lengthb[] = LITTLE_ENDIAN_INT(message_length);

    int offset = params->sent_bytes - size_to_tag;
    int left = 4 - offset;

    params->sent_bytes += send(socket, &lengthb + offset, left, 0);
  }

  if (params->sent_bytes >= size_to_length &&
      params->sent_bytes < size_to_message) {
    size_t offset = params->sent_bytes - size_to_length;
    size_t left = message_length - offset;

    params->sent_bytes += send(socket, message + offset, left, 0);
  }

  if (params->sent_bytes == final_size) {
    struct client *client = CLIENT(params);

    if (params->callback)
      params->callback(client);

    free(params);
  } else {
    Debug(NetSender, "Partial send (%d/%d) for tag %x, recursing\n",
          params->sent_bytes, final_size, tag);
    struct message *message =
        message_new(params, (void *)recurse_handler, "netsender/ask/rec");

    event_loop_add_message(message);
  }
}

static void send_ask_to_1_handler(struct message_handler_params *params) {
  send_ask_to_x_handler(params, ASK_TO_01, send_ask_to_1_handler);
}
static void send_ask_to_2_handler(struct message_handler_params *params) {
  send_ask_to_x_handler(params, ASK_TO_02, send_ask_to_2_handler);
}

static void
send_score_x_handler(struct message_handler_params *params, unsigned char tag,
                     int score,
                     void (*recurse_handler)(struct message_handler_params *)) {
  const int size_to_tag = 1;
  const int size_to_score = size_to_tag + 4;
  const int final_size = size_to_score;

  int socket = SOCKET(params);

  if (params->sent_bytes == 0) {
    unsigned char buffer[] = {tag};
    params->sent_bytes += send(socket, buffer, 1, 0);
  }

  if (params->sent_bytes >= size_to_tag && params->sent_bytes < size_to_score) {
    unsigned char scoreb[] = LITTLE_ENDIAN_INT(score);

    int offset = params->sent_bytes - size_to_tag;
    int left = 4 - offset;

    params->sent_bytes += send(socket, &scoreb + offset, left, 0);
  }

  if (params->sent_bytes == final_size) {
    struct client *client = CLIENT(params);

    if (params->callback)
      params->callback(client);

    free(params);
  } else {
    Debug(NetSender, "Partial send (%d/%d) for tag %x, recursing\n",
          params->sent_bytes, final_size, tag);
    struct message *message =
        message_new(params, (void *)recurse_handler, "netsender/score/rec");

    event_loop_add_message(message);
  }
}

static void send_score_1_handler(struct message_handler_params *params) {
  int score = GAME(params)->player_1.score;
  send_score_x_handler(params, SCORE_01, score, send_score_1_handler);
}

static void send_score_2_handler(struct message_handler_params *params) {
  int score = GAME(params)->player_2.score;
  send_score_x_handler(params, SCORE_02, score, send_score_2_handler);
}

static void to_event_loop(struct client *client, void callback(struct client *),
                          void (*handler)(struct message_handler_params *),
                          char *description) {
  struct message_handler_params *params =
      malloc(sizeof(struct message_handler_params));
  params->callback = callback;
  params->client = client;
  params->sent_bytes = 0;

  struct message *message = message_new(params, (void *)handler, description);
  event_loop_add_message(message);
}

void send_deny_abort(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_deny_abort_handler,
                "netsender/deny_abort");
}
void send_deny_retry(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_deny_retry_handler,
                "netsender/deny_retry");
}
void send_game_found(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_game_found_handler,
                "netsender/game_found");
}
void send_joined_game(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_joined_game_handler,
                "netsender/joined_game");
}
void send_ask_to_1(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_ask_to_1_handler, "netsender/ask_to_1");
}
void send_ask_to_2(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_ask_to_2_handler, "netsender/ask_to_2");
}
void send_pass_to_1(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_pass_to_1_handler,
                "netsender/pass_to_1");
}
void send_pass_to_2(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_pass_to_2_handler,
                "netsender/pass_to_2");
}
void send_score_1(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_score_1_handler, "netsender/score_1");
}
void send_score_2(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_score_2_handler, "netsender/score_2");
}
void send_game_end(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_game_end_handler, "netsender/game_end");
}
void send_game_aborted(struct client *client, void callback(struct client *)) {
  to_event_loop(client, callback, send_game_aborted_handler,
                "netsender/game_aborted");
}
