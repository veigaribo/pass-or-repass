#ifndef H_PASS_OR_REPASS_EVENT_LOOP
#define H_PASS_OR_REPASS_EVENT_LOOP

#include "message.h"
#include "thr_queue.h"

#define WORKER_THREADS 8

void event_loop_init();
void event_loop_add_message(struct message *message);
void event_loop_terminate();

#endif
