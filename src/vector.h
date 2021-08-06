#ifndef H_PASS_OR_REPASS_VECTOR
#define H_PASS_OR_REPASS_VECTOR

struct vector {
  void **data;
  int length;
  int capacity;
};

void vector_init(struct vector *dest, int length, int capacity);
void vector_grow(struct vector *vec);
void vector_fit(struct vector *vec, int length);
void vector_push(struct vector *vec, void *data);
void *vector_rm_by_addr(struct vector *vec, void *addr);
void *vector_pop(struct vector *vec, int index);
void vector_free(struct vector *vec);

#endif
