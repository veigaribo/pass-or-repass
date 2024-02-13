#include "event_handlers.h"
#include "debug.h"
#include "mm.h"
#include "net_sender.h"
#include "state.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void (*handlers[])(struct message_handler_params *) = {
    handle_deny_abort,  handle_deny_retry, handle_join_queue, handle_quit_queue,
    handle_accept_game, handle_ask_again,  handle_answer_a,   handle_answer_b,
    handle_answer_c,    handle_set_name,   handle_pass,       handle_quit_game};

void (*get_handler(enum net_in_event_type event_type))(
    struct message_handler_params *) {

  return handlers[event_type];
}

// Should we set `busy` now or only after all async stuff is done?..
#define CLEANUP                                                                \
  struct client *cleanup_client =                                              \
      state_get_client(params->state, params->event.client_socket);            \
  cleanup_client->busy = false;                                                \
  free(params)

#define CLIENT(Params)                                                         \
  state_get_client(params->state, params->event.client_socket)

// As a general rule, this functions should only check for the existence
// of things and delegate everything else

void handle_join_queue(struct message_handler_params *params) {
  struct client *client = CLIENT(params);
  Debug(EventHandlers, "Client %d joining queue\n", client->socket);

  mm_client_join(client);
  CLEANUP;
}

void handle_quit_queue(struct message_handler_params *params) {
  struct client *client = CLIENT(params);
  Debug(EventHandlers, "Client %d quitting queue\n", client->socket);

  mm_client_quit(client);
  CLEANUP;
}

void handle_accept_game(struct message_handler_params *params) {
  struct client *client = CLIENT(params);
  Debug(EventHandlers, "Client %d accepting game\n", client->socket);

  if (client->ready_check) {
    ready_check_accept(client);
  } else {
    send_deny_abort(client, NULL);
  }

  CLEANUP;
}

void handle_deny_abort(struct message_handler_params *params) {
  // We just wait for the client to disconnect for now
}

void handle_deny_retry(struct message_handler_params *params) {
  // Unused for now
}

void handle_ask_again(struct message_handler_params *params) {
  // Unused for now
}

void handle_answer_a(struct message_handler_params *params) {
  struct client *client = CLIENT(params);
  Debug(EventHandlers, "Client %d answered A\n", client->socket);

  if (client->player == NULL || client->player->game == NULL) {
    send_deny_abort(client, NULL);
    return;
  }

  game_answer(client, OPTION_A);
  CLEANUP;
}

void handle_answer_b(struct message_handler_params *params) {
  struct client *client = CLIENT(params);
  Debug(EventHandlers, "Client %d answered B\n", client->socket);

  if (client->player == NULL || client->player->game == NULL) {
    send_deny_abort(client, NULL);
    return;
  }

  game_answer(client, OPTION_B);
  CLEANUP;
}

void handle_answer_c(struct message_handler_params *params) {
  struct client *client = CLIENT(params);
  Debug(EventHandlers, "Client %d answered C\n", client->socket);

  if (client->player == NULL || client->player->game == NULL) {
    send_deny_abort(client, NULL);
    return;
  }

  game_answer(client, OPTION_C);
  CLEANUP;
}

void handle_set_name(struct message_handler_params *params) {
  // Unused for now
}

void handle_pass(struct message_handler_params *params) {
  struct client *client = CLIENT(params);
  Debug(EventHandlers, "Client %d passing\n", client->socket);

  if (client->player == NULL || client->player->game == NULL) {
    send_deny_abort(client, NULL);
    return;
  }

  game_pass(client);
  CLEANUP;
}

void handle_quit_game(struct message_handler_params *params) {
  struct client *client = CLIENT(params);
  Debug(EventHandlers, "Client %d quit game\n", client->socket);

  if (client->player != NULL && client->player->game != NULL) {
    game_abort(client);
  }

  CLEANUP;
}

// Doesn't handle anything client-related since it is assumed the
// client is gone
#define DISCONNECT_CLEANUP free(params)

static pthread_mutex_t global_disconnect_mutex = PTHREAD_MUTEX_INITIALIZER;

// These ones break the rule for now

void handle_disconnect(struct message_handler_params *params) {
  pthread_mutex_lock(&global_disconnect_mutex);

  struct state *state = params->state;
  struct net_event *event = &params->event;

  struct client *client = state_get_client(state, event->client_socket);

  if (client == NULL) {
    Debug(EventHandlers, "Null client disconnected..? %d\n",
          event->client_socket);
    goto end;
  }

  Debug(EventHandlers, "Client disconnecting %d\n", client->socket);
  client->socket = -1;

  mm_try_client_quit(client);

  if (client->ready_check != NULL) {
    ready_check_abort(client);
  }

  if (client->player != NULL && client->player->game != NULL) {
    game_abort(client);
  }

  thr_vector_rm_by_addr(&state->clients, client);

  close(client->socket);
  client_free(client);
  free(client);
  DISCONNECT_CLEANUP;

end:
  pthread_mutex_unlock(&global_disconnect_mutex);
}

void handle_error(struct message_handler_params *params) {
  handle_disconnect(params);
}
