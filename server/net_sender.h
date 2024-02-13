#ifndef H_PASS_OR_REPASS_NET_SENDER
#define H_PASS_OR_REPASS_NET_SENDER

#include "state.h"

void send_deny_abort(struct client *client, void callback(struct client *));
void send_deny_retry(struct client *client, void callback(struct client *));
void send_game_found(struct client *client, void callback(struct client *));
void send_joined_game(struct client *client, void callback(struct client *));
void send_ask_to_1(struct client *client, void callback(struct client *));
void send_ask_to_2(struct client *client, void callback(struct client *));
void send_pass_to_1(struct client *client, void callback(struct client *));
void send_pass_to_2(struct client *client, void callback(struct client *));
void send_score_1(struct client *client, void callback(struct client *));
void send_score_2(struct client *client, void callback(struct client *));
void send_game_end(struct client *client, void callback(struct client *));
void send_game_aborted(struct client *client, void callback(struct client *));

#endif
