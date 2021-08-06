#ifndef H_PASS_OR_REPASS_QUESTION
#define H_PASS_OR_REPASS_QUESTION

enum question_answer { option_a, option_b, option_c };

struct question {
  int id;
  char *text;
  int length;

  enum question_answer correct_answer;
};

struct question *get_question();

#endif
