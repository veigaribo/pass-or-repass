#include "list.h"

#include <stdlib.h>

void list_free(struct list *list, bool free_values) {
  struct list *cursor = list;

  struct list *next;
  while (cursor != NULL) {
    next = cursor->next;

    if (free_values) {
      free(cursor->data);
    }

    free(cursor->data);
    cursor = next;
  }
}
