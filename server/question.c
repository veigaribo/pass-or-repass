#include "question.h"
#include "id.h"
#include <string.h>

#define QUESTION1_TEXT                                                         \
  "What is the most common cause of massive rectal bleeding in an adult?\n"    \
  "A) Polyps\n"                                                                \
  "B) Diverticulosis\n"                                                        \
  "C) Mesentric thrombosis\n"

#define QUESTION2_TEXT                                                         \
  "The scientific name of an organism is Dendroica pennsylvanica. Another "    \
  "organism named Dendroica petechiae belongs to the same\n"                   \
  "A) Family\n"                                                                \
  "B) Order\n"                                                                 \
  "C) Genus"

#define QUESTION3_TEXT                                                         \
  "If  (p/q)/r = x/(q/r) , where / means division, find x.\n"                  \
  "A)  p/(r*r) , where * means multiplication\n"                               \
  "B)  p*q/r , where * means multiplication\n"                                 \
  "C)  p"

#define QUESTION4_TEXT                                                         \
  "The length of each side of a square is increased by 3m. The area of the "   \
  "'new' square is 39 meters squared more than that of the original square. "  \
  "How long are the sides of the 'new' square?\n"                              \
  "A) 7m\n"                                                                    \
  "B) 8m\n"                                                                    \
  "C) 9m"

#define QUESTION5_TEXT                                                         \
  "What property is required for a binary operation to form a semigroup with " \
  "a given set?\n"                                                             \
  "A) Associativity\n"                                                         \
  "B) Commutativity\n"                                                         \
  "C) Divisibility"

#define QUESTION6_TEXT                                                         \
  "Which of the following numbers is a quadratic residue modulo 7?\n"          \
  "A) 6\n"                                                                     \
  "B) 5\n"                                                                     \
  "C) 4"

#define QUESTION7_TEXT                                                         \
  "The name \"Mathematics\" is derived from a verb in ancient Greek. The "     \
  "English translation of that verb usually is:\n"                             \
  "A) To count\n"                                                              \
  "B) To teach\n"                                                              \
  "C) To measure"

struct question question1 = {1, QUESTION1_TEXT, strlen(QUESTION1_TEXT),
                             OPTION_B};
struct question question2 = {2, QUESTION2_TEXT, strlen(QUESTION2_TEXT),
                             OPTION_C};
struct question question3 = {3, QUESTION3_TEXT, strlen(QUESTION3_TEXT),
                             OPTION_A};
struct question question4 = {4, QUESTION4_TEXT, strlen(QUESTION4_TEXT),
                             OPTION_B};
struct question question5 = {5, QUESTION5_TEXT, strlen(QUESTION5_TEXT),
                             OPTION_A};
struct question question6 = {6, QUESTION6_TEXT, strlen(QUESTION6_TEXT),
                             OPTION_C};
struct question question7 = {7, QUESTION7_TEXT, strlen(QUESTION7_TEXT),
                             OPTION_B};

struct question *questions[] = {&question1, &question2, &question3, &question4,
                                &question5, &question6, &question7};

struct question *get_question(size_t *used_already, size_t used_count) {
  const size_t question_count = sizeof questions / sizeof questions[0];

  struct question *options[question_count];
  size_t options_i = 0;

  for (size_t i = 0; i < question_count; ++i) {
    struct question *question = questions[i];

    for (size_t j = 0; j < used_count; ++j) {
      size_t used = used_already[j];

      if (question->id == used) {
        goto next_question;
      }
    }

    // Question not used
    options[options_i++] = question;
  next_question:;
  }

  size_t options_count = options_i;
  long index = id_get() % options_count;

  return options[index];
}
