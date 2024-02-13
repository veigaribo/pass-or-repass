#include "message.h"
#include "event_loop.h"
#include <stdlib.h>

struct message *message_new(void *params, void (*callback)(void *),
                            char *description) {
  struct message *message = malloc(sizeof(struct message));
  message->callback = callback;
  message->params = params;
  message->description = description;

  return message;
}
