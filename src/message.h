#ifndef H_PASS_OR_REPASS_MESSAGE
#define H_PASS_OR_REPASS_MESSAGE

struct message {
  void (*callback)(void *data);
  void *params;
};

#endif
