#include "protocol.h"

#include <stdbool.h>
#include <string.h>

int nparams[256];
bool initted = false;

void nparams_init() {
  memset(&nparams, 0, 0xff);

  nparams[ACK] = 0;
  nparams[DENY] = 0;
  nparams[JOIN_QUEUE] = 0;
  nparams[QUIT_QUEUE] = 0;
  nparams[RESUME_GAME] = 1;
  nparams[ASK_AGAIN] = 0;
  nparams[ANSWER_A] = 0;
  nparams[ANSWER_B] = 0;
  nparams[ANSWER_C] = 0;
  nparams[PASS] = 0;
  nparams[QUIT_GAME] = 0;
  nparams[GAME_FOUND] = 0;
  nparams[JOINED_GAME] = 2;
  nparams[ASK_TO_01] = 2;
  nparams[ASK_TO_02] = 2;
  nparams[PASS_TO_01] = 0;
  nparams[PASS_TO_02] = 0;
  nparams[SCORE_01] = 1;
  nparams[SCORE_02] = 1;
  nparams[GAME_ABORTED] = 0;

  initted = true;
}

int nparams_get(unsigned char event) {
  if (!initted) {
    nparams_init();
  }

  return nparams[event];
}
