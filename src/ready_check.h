#ifndef H_PASS_OR_REPASS_READY_CHECK
#define H_PASS_OR_REPASS_READY_CHECK

struct ready_check {
  struct client *c1;
  struct client *c2;

  int accepteds;
};

#endif
