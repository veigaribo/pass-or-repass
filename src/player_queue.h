#ifndef H_PASS_OR_REPASS_PLAYER_QUEUE
#define H_PASS_OR_REPASS_PLAUER_QUEUE

struct client {
  int socket;
};

struct client_connected_params {
  struct thr_vector *clients;
  int socket;
};

void on_client_connected(struct client_connected_params *data);

#endif
