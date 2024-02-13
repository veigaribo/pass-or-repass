#include "net_parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "debug.h"
#include "protocol.h"
#include "question.h"
#include "vector.h"

void net_parser_init(struct net_parser *state, int client_socket) {
  state->client_socket = client_socket;

  memset(state->buffer, 0, NET_PARSER_BUFFER_CAP);
  state->buffer_start = 0;
  state->buffer_length = 0;
  state->backpressured = false;
}

// Assumes there will be enough space in the buffer
void net_parser_update(struct net_parser *state, int socket) {
  // Sending too much stuff. Ignored
  if (state->backpressured)
    return;

  size_t buffer_end =
      (state->buffer_start + state->buffer_length) % NET_PARSER_BUFFER_CAP;

  unsigned char *head = &state->buffer[buffer_end];
  int received = recv(socket, head, NET_PARSER_LOAD_COUNT, 0);

  state->buffer_length += received;

  // Buffer cannibalizing itself
  if (state->buffer_length > NET_PARSER_BUFFER_CAP) {
    state->backpressured = true;
    return;
  }

  if (received < NET_PARSER_LOAD_COUNT) {
    // Done
    return;
  }

  net_parser_update(state, socket);
}

static void consume_data(struct net_parser *state, size_t amount) {
  state->buffer_start += amount;
  state->buffer_length -= amount;
}

enum parse_status { PARSE_SUCCESS, PARSE_INCOMPLETE, PARSE_FAILED };

static void parse_deny_abort(struct net_parser *state, struct net_event *event,
                             enum parse_status *status);
static void parse_deny_retry(struct net_parser *state, struct net_event *event,
                             enum parse_status *status);
static void parse_join_queue(struct net_parser *state, struct net_event *event,
                             enum parse_status *status);
static void parse_quit_queue(struct net_parser *state, struct net_event *event,
                             enum parse_status *status);
static void parse_accept_game(struct net_parser *state, struct net_event *event,
                              enum parse_status *status);
static void parse_ask_again(struct net_parser *state, struct net_event *event,
                            enum parse_status *status);
static void parse_answer_a(struct net_parser *state, struct net_event *event,
                           enum parse_status *status);
static void parse_answer_b(struct net_parser *state, struct net_event *event,
                           enum parse_status *status);
static void parse_answer_c(struct net_parser *state, struct net_event *event,
                           enum parse_status *status);
static void parse_pass(struct net_parser *state, struct net_event *event,
                       enum parse_status *status);
static void parse_quit_game(struct net_parser *state, struct net_event *event,
                            enum parse_status *status);
static void parse_set_name(struct net_parser *state, struct net_event *event,
                           enum parse_status *status);

bool net_parser_get_event(struct net_parser *state, struct net_event *event) {
  // We always try to read the entire thing. If there is not enough,
  // we wait.
  unsigned char *head = &state->buffer[state->buffer_start];
  unsigned char tag = head[0];

  Debug(NetParser, "Read event type %d\n", tag);
  enum parse_status status;

  switch (tag) {
  case DENY_ABORT:
    parse_deny_abort(state, event, &status);
    break;
  case DENY_RETRY:
    parse_deny_retry(state, event, &status);
    break;
  case JOIN_QUEUE:
    parse_join_queue(state, event, &status);
    break;
  case QUIT_QUEUE:
    parse_quit_queue(state, event, &status);
    break;
  case ACCEPT_GAME:
    parse_accept_game(state, event, &status);
    break;
  case ASK_AGAIN:
    parse_ask_again(state, event, &status);
    break;
  case ANSWER_A:
    parse_answer_a(state, event, &status);
    break;
  case ANSWER_B:
    parse_answer_b(state, event, &status);
    break;
  case ANSWER_C:
    parse_answer_c(state, event, &status);
    break;
  case PASS:
    parse_pass(state, event, &status);
    break;
  case QUIT_GAME:
    parse_quit_game(state, event, &status);
    break;
  case SET_NAME:
    parse_set_name(state, event, &status);
    break;
  case 0:
    consume_data(state, 1);
    event->client_socket = state->client_socket;
    event->type = IN_DISCONNECT;
    return true;
  default:
    consume_data(state, 1);
    event->client_socket = state->client_socket;
    event->type = IN_ERROR_BOGUS;
    return true;
  }

  switch (status) {
  case PARSE_SUCCESS:
    return true;
  case PARSE_FAILED:
    event->type = IN_ERROR_BOGUS;
  case PARSE_INCOMPLETE:
    return false;
  }
}

#define TRIVIAL_PARSE(Type)                                                    \
  event->client_socket = state->client_socket;                                 \
  event->type = Type;                                                          \
  *status = PARSE_SUCCESS;                                                     \
  consume_data(state, 1)

static void parse_deny_abort(struct net_parser *state, struct net_event *event,
                             enum parse_status *status) {
  TRIVIAL_PARSE(IN_DENY_ABORT);
}
static void parse_deny_retry(struct net_parser *state, struct net_event *event,
                             enum parse_status *status) {
  TRIVIAL_PARSE(IN_DENY_RETRY);
}
static void parse_join_queue(struct net_parser *state, struct net_event *event,
                             enum parse_status *status) {
  TRIVIAL_PARSE(IN_JOIN_QUEUE);
}
static void parse_quit_queue(struct net_parser *state, struct net_event *event,
                             enum parse_status *status) {
  TRIVIAL_PARSE(IN_QUIT_QUEUE);
}
static void parse_accept_game(struct net_parser *state, struct net_event *event,
                              enum parse_status *status) {
  TRIVIAL_PARSE(IN_ACCEPT_GAME);
}
static void parse_ask_again(struct net_parser *state, struct net_event *event,
                            enum parse_status *status) {
  TRIVIAL_PARSE(IN_ASK_AGAIN);
}
static void parse_answer_a(struct net_parser *state, struct net_event *event,
                           enum parse_status *status) {
  TRIVIAL_PARSE(IN_ANSWER_A);
}
static void parse_answer_b(struct net_parser *state, struct net_event *event,
                           enum parse_status *status) {
  TRIVIAL_PARSE(IN_ANSWER_B);
}
static void parse_answer_c(struct net_parser *state, struct net_event *event,
                           enum parse_status *status) {
  TRIVIAL_PARSE(IN_ANSWER_C);
}
static void parse_pass(struct net_parser *state, struct net_event *event,
                       enum parse_status *status) {
  TRIVIAL_PARSE(IN_PASS);
}
static void parse_quit_game(struct net_parser *state, struct net_event *event,
                            enum parse_status *status) {
  TRIVIAL_PARSE(IN_QUIT_GAME);
}

static void parse_set_name(struct net_parser *state, struct net_event *event,
                           enum parse_status *status) {
  if (state->buffer_length < 2) {
    *status = PARSE_INCOMPLETE;
    return;
  }

  // 1 for tag
  size_t to_be_consumed = 1;
  unsigned char *head = &state->buffer[state->buffer_start];

  size_t size = head[1];
  to_be_consumed += 1;

  if (size > SET_NAME_MAX_SIZE) {
    *status = PARSE_FAILED;
    return;
  }

  if (state->buffer_length < 2 + size) {
    *status = PARSE_INCOMPLETE;
    return;
  }

  event->client_socket = state->client_socket;
  event->type = SET_NAME;
  event->params[0].data = head + to_be_consumed;
  event->params[0].count = size;
  *status = PARSE_SUCCESS;

  to_be_consumed += size;
  consume_data(state, to_be_consumed);
}
