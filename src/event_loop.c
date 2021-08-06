#include "event_loop.h"

#include <stdio.h>
#include <stdlib.h>

struct worker_thread_controls thread_controls;
struct thr_queue event_queue;
int dispatcher_thread;

void *work(void *param) {
  struct worker_data *data = param;
  int worker_index = data->index;

  pthread_mutex_t *stop = &thread_controls.worker_stops[worker_index];

  bool *am_working = &thread_controls.worker_is_working[worker_index];

  printf("Thread %d working\n", worker_index);

  struct message *message =
      thread_controls.worker_working_messages[worker_index];

  printf("Thread %d got message\n", worker_index);
  message->callback(message->params);

  thread_controls.worker_working_messages[worker_index] = NULL;

  // Done
  free(message);
  *am_working = false;

  return NULL;
}

void *dispatch(void *param) {
  struct message *message;
  printf("Dispatching\n");

  while ((message = thr_queue_dequeue(&event_queue))) {
    printf("Dispatcher found message\n");

    while (true) {
      thread_controls.current_thread =
          (thread_controls.current_thread + 1) % WORKER_THREADS;

      printf("Looking for thread, current one %d\n",
             thread_controls.current_thread);

      if (!thread_controls.worker_is_working[thread_controls.current_thread]) {
        thread_controls
            .worker_working_messages[thread_controls.current_thread] = message;

        thread_controls.worker_is_working[thread_controls.current_thread] =
            true;

        pthread_create(
            &thread_controls.worker_threads[thread_controls.current_thread],
            NULL, work,
            &thread_controls.worker_params[thread_controls.current_thread]);
        break;
      }

      printf("Thread %d busy\n", thread_controls.current_thread);
    }

    printf("Event queue back now: %p\n", event_queue.data->back);
  }

  printf("Dispatched\n");
  thread_controls.is_dispatching = false;
  return NULL;
}

struct thr_queue *event_loop_get() {
  return &thread_controls.event_queue;
}

void event_loop_init() {
  thr_queue_init(&event_queue);

  for (int i = 0; i < WORKER_THREADS; i++) {
    thread_controls.worker_is_working[i] = false;
    thread_controls.worker_params[i].index = i;
  }
}

void event_loop_add_message(struct message *message) {
  thr_queue_enqueue(&event_queue, message);

  if (!thread_controls.is_dispatching) {
    thread_controls.is_dispatching = true;
    pthread_create(&thread_controls.dispatcher_thread, NULL, dispatch, NULL);
  }
}

void event_loop_terminate() { thr_queue_free(&event_queue, true); }
