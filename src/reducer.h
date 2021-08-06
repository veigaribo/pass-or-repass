#ifndef H_PASS_OR_REPASS_REDUCER
#define H_PASS_OR_REPASS_REDUCER

#include "interpreter.h"
#include "question.h"

struct reducer_params {
  struct handle_params *handle_params;
  void *extra;
};

struct make_ready_check_extra {
  struct client *c1;
  struct client *c2;
};

// Still using handle_params directly because I don't have
// much time to change them
void join_queue(struct handle_params *params);
void quit_queue(struct handle_params *params);

void make_ready_check(struct reducer_params params);
void maybe_make_game(struct reducer_params params);
void handle_ack(struct reducer_params params);
void answer_question(struct reducer_params params, enum question_answer answer);
void pass_question(struct reducer_params params);
void disconnect_client(struct reducer_params params);

#endif
