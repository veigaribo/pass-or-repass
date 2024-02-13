#ifndef H_PASS_OR_REPASS_MM
#define H_PASS_OR_REPASS_MM

#include "state.h"

// MM stands for match making

#define MM_POLLING_INTERVAL_S 2

void mm_loop_init(struct state *state);

void mm_client_join(struct client *client);
void mm_client_quit(struct client *client);
struct client *mm_try_client_quit(struct client *client);

#endif
