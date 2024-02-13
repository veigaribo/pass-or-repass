#ifndef H_PASS_OR_REPASS_MESSAGE
#define H_PASS_OR_REPASS_MESSAGE

struct message {
  void (*callback)(void *data);
  void *params;

  // So it's somewhat possible to debug
  char *description;
};

struct message *message_new(void *params, void (*callback)(void *),
                            char *description);

#endif
