#include "id.h"

unsigned long current_id = 0;

long id_get() { return ++current_id; }

char ctochar(unsigned char c) {
  int numberity = !(c < 10);

  // If < 10, is number, otherwise lowercase letter
  return c + 48 + (39 * numberity);
}

void id_get_str(char *dest) {
  const int max = 29;
  unsigned long base = id_get() % max;
  unsigned char c = base;

  for (int i = 0; i < ID_STR_LENGTH; i++) {
    // ((c + 1) * 33) % max
    c = (((c + i) << 5) + c) % max;
    dest[i] = ctochar(c);
  }
}
