#ifndef H_PASS_OR_REPASS_LIST
#define H_PASS_OR_REPASS_LIST

#include <stdbool.h>

struct list {
  void *data;
  struct list *next;
  struct list *previous;
};

void list_free(struct list *list, bool free_values);

#endif
