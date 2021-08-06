#ifndef H_PASS_OR_REPASS_EVENT_LOOP
#define H_PASS_OR_REPASS_EVENT_LOOP

#include "message.h"
#include "thr_queue.h"

#define WORKER_THREADS 8

struct worker_data {
  int index;
};

struct terminator_data {
  int index;
};

// Probably should not expose this but whatever
struct worker_thread_controls {
  struct thr_queue event_queue;

  struct worker_data worker_params[WORKER_THREADS];
  struct message *worker_working_messages[WORKER_THREADS];

  // Do the processing
  pthread_t worker_threads[WORKER_THREADS];

  // Stop the worker threads when they allow it
  pthread_t terminator_threads[WORKER_THREADS];

  // Effectively starts the worker threads
  pthread_t dispatcher_thread;

  // Worker threads hold on to semaphores to start working
  pthread_mutex_t worker_semaphores[WORKER_THREADS];
  // Terminator threads hold on to stops to know when to close the semaphore
  pthread_mutex_t worker_stops[WORKER_THREADS];

  // Keep track of which threads are already working
  bool worker_is_working[WORKER_THREADS];

  // We only need to invoke the dispatching thread if it is not dispatching
  // already
  bool is_dispatching;

  // Do a round-robin on the workers to avoid checking occupied ones all the
  // time
  int current_thread;
};

void event_loop_init();
struct thr_queue *event_loop_get();
void event_loop_add_message(struct message *message);
void event_loop_terminate();

#endif
