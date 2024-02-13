#ifndef H_PASS_OR_REPASS_EVENT_HANDLERS
#define H_PASS_OR_REPASS_EVENT_HANDLERS

#include "net_parser.h"

struct message_handler_params {
  struct state *state;
  struct net_event event;
};

// See net_parser.h
void handle_deny_abort(struct message_handler_params *params);
void handle_deny_retry(struct message_handler_params *params);
void handle_join_queue(struct message_handler_params *params);
void handle_quit_queue(struct message_handler_params *params);
void handle_accept_game(struct message_handler_params *params);
void handle_ask_again(struct message_handler_params *params);
void handle_answer_a(struct message_handler_params *params);
void handle_answer_b(struct message_handler_params *params);
void handle_answer_c(struct message_handler_params *params);
void handle_set_name(struct message_handler_params *params);
void handle_pass(struct message_handler_params *params);
void handle_quit_game(struct message_handler_params *params);

void handle_disconnect(struct message_handler_params *params);
void handle_error(struct message_handler_params *params);

// Returns the handler for the in event type...
void (*get_handler(enum net_in_event_type))(struct message_handler_params *);

#endif
