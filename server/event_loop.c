#include "event_loop.h"

#include "debug.h"
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

struct worker_data {
  size_t index;
  struct message *to_handle;

  sem_t semaphore;
};

struct thr_queue event_queue;

pthread_t dispatcher_thread;
sem_t dispatcher_semaphore;

struct worker_data worker_data[WORKER_THREADS];
pthread_t worker_threads[WORKER_THREADS];
bool worker_thread_working[WORKER_THREADS];

void *work(void *param) {
  struct worker_data *data = param;
  data->to_handle = NULL;
  sem_init(&data->semaphore, 0, 0);

  while (true) {
    sem_wait(&data->semaphore);
    struct message *message = data->to_handle;

    Debug(EventLoop, "Thread %lu handling message %s\n", data->index,
          message->description);

    message->callback(message->params);

    free(message);
    data->to_handle = NULL;

    sched_yield();
  }

  return NULL;
}

void *dispatch(void *param) {
  // Round robin
  size_t index = 0;

  while (true) {
    sem_wait(&dispatcher_semaphore);
    struct message *message = thr_queue_dequeue(&event_queue);

    struct worker_data *worker = &worker_data[index];
    index = (index + 1) % WORKER_THREADS;

    worker->to_handle = message;
    sem_post(&worker->semaphore);

    sched_yield();
  }

  return NULL;
}

void event_loop_init() {
  thr_queue_init(&event_queue);

  sem_init(&dispatcher_semaphore, 0, 0);
  pthread_create(&dispatcher_thread, NULL, dispatch, NULL);

  for (size_t i = 0; i < WORKER_THREADS; ++i) {
    worker_data[i].index = i;
    pthread_create(&worker_threads[i], NULL, work, &worker_data[i]);
  }
}

void event_loop_add_message(struct message *message) {
  thr_queue_enqueue(&event_queue, message);
  sem_post(&dispatcher_semaphore);
}

void event_loop_terminate() {
  // Too hard. This shouldn't be called before the process exists
  // anyway.
}
