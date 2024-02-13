#ifndef H_PASS_OR_REPASS_INTERPRETER
#define H_PASS_OR_REPASS_INTERPRETER

#include "state.h"

#define NET_PARSER_BUFFER_CAP 48
#define NET_PARSER_LOAD_COUNT (NET_PARSER_BUFFER_CAP / 2)

// In general does not know about the protocol
struct net_parser {
  int client_socket;

  // Circular buffer
  unsigned char buffer[NET_PARSER_BUFFER_CAP];
  size_t buffer_start;
  size_t buffer_length;

  // Persists until reconnection
  bool backpressured;
};

enum net_in_event_type {
  IN_DENY_ABORT = 0,
  IN_DENY_RETRY = 1,

  IN_JOIN_QUEUE = 2,
  IN_QUIT_QUEUE = 3,
  IN_ACCEPT_GAME = 4,
  IN_ASK_AGAIN = 5,
  IN_ANSWER_A = 6,
  IN_ANSWER_B = 7,
  IN_ANSWER_C = 8,
  IN_SET_NAME = 9,
  IN_PASS = 10,
  IN_QUIT_GAME = 11,

  IN_DISCONNECT = 1 << 8,
  IN_ERROR_BOGUS = 2 << 8,
};

// IMPORTANT: These strings will only live while the buffer does not
// wrap back to the same place. Therefore do not update data while
// handling previous stuff
struct net_event_param {
  unsigned char *data;
  size_t count;
};

struct net_event {
  int client_socket;
  enum net_in_event_type type;

  struct net_event_param params[2];
};

void net_parser_init(struct net_parser *state, int client_socket);

// Reads and store network data
void net_parser_update(struct net_parser *state, int socket);

// Fills net_event structs, maybe more than one per update
bool net_parser_get_event(struct net_parser *state, struct net_event *event);

#endif
