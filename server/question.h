#ifndef H_PASS_OR_REPASS_QUESTION
#define H_PASS_OR_REPASS_QUESTION

#include <stddef.h>

enum question_answer { OPTION_A, OPTION_B, OPTION_C };

struct question {
  size_t id;
  char *text;
  size_t length;

  enum question_answer correct_answer;
};

// Will not repeat one that has already been used
struct question *get_question(size_t *used_already_ids, size_t used_count);

#endif
