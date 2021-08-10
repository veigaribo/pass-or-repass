#include "id.h"

unsigned long current_id = 0;

long id_get() { return ++current_id; }

char ctochar(unsigned char c) {
  int isalpha = c > 9;

  // If < 10, is number, otherwise lowercase letter
  return c + 48 + (39 * isalpha);
}

void id_get_str(char *dest) {
  const int max = 36;
  const int magic = 0x3e779921;
  unsigned long base = id_get();
  unsigned long c = base;

  for (int i = 0; i < ID_STR_LENGTH; i++) {
    c ^= base + magic + (c << 6) + (c >> 2);
    dest[i] = ctochar(c % max);
  }
}
