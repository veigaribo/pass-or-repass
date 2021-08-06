#include "interpreter.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "protocol.h"
#include "question.h"
#include "reducer.h"
#include "vector.h"

#define BUF_SIZE 1024

struct game_event event_get(int socket) {
  unsigned char event = 0;

  int bytes_read = recv(socket, &event, 1, 0);

  printf("First recv %d (%d)\n", event, bytes_read);

  int nparams = nparams_get(event);
  char **params = malloc(sizeof(char *) * 2);

  struct vector vec;
  vector_init(&vec, 0, BUF_SIZE);

  int read, total_length = 0, offset = 0, current_param = 0;
  unsigned char buffer[BUF_SIZE];
  char *param;
  bool found_param = false;

  printf("NParams for event %d: %d\n", event, nparams);
  printf("Buffer at: %p\n", &buffer);

  while (current_param < nparams) {
    int bytes_read = recv(socket, buffer, BUF_SIZE, 0);
    total_length += bytes_read;

    vector_fit(&vec, total_length);

    memcpy(vec.data[offset], &buffer, bytes_read);
    vec.length = total_length;

    for (int i = offset; i < total_length; i++) {
      if (buffer[i] == 0) {
        printf("Found separator: i %d, offset %d, br %d, tl %d", i, offset,
               bytes_read, total_length);

        found_param = true;

        param = malloc(sizeof(char) * (i + 1));
        memcpy(&param, &vec.data, i + 1);

        params[current_param] = param;

        printf("Parameter[%d]: %s", current_param, params[current_param]);

        current_param++;

        total_length = 0;
        offset = 0;
      }
    }

    if (found_param) {
      found_param = false;
    } else {
      offset = total_length;
    }
  }

  struct game_event result;
  result.client_socket = socket;
  result.type = event;
  result.params = params;
  result.nparams = nparams;

  return result;
}

void event_free(struct game_event event) {
  for (int i = 0; i < event.nparams; i++) {
    free(event.params[i]);
  }

  free(event.params);
}

void handle(void *params) {
  struct handle_params *cparams = params;
  struct game_event event = cparams->event;

  struct reducer_params reducer_params;
  reducer_params.handle_params = cparams;
  reducer_params.extra = NULL;

  printf("Handling event %d c:\n", event.type);

  switch (event.type) {
    case 0:
      disconnect_client(reducer_params);
      break;
    case JOIN_QUEUE:
      join_queue(cparams);
      break;
    case QUIT_QUEUE:
      quit_queue(cparams);
      break;
    case ACK:
      handle_ack(reducer_params);
      break;
    case ANSWER_A:
      answer_question(reducer_params, option_a);
      break;
    case ANSWER_B:
      answer_question(reducer_params, option_b);
      break;
    case ANSWER_C:
      answer_question(reducer_params, option_c);
      break;
    case PASS:
      pass_question(reducer_params);
      break;
  }
}
