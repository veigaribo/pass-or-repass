#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "event_loop.h"
#include "interpreter.h"
#include "message.h"
#include "mm.h"
#include "state.h"
#include "thr_hashmap.h"
#include "thr_queue.h"
#include "thr_vector.h"

#define WORKER_THREADS 8
#define MAX_EPOLL_EVENTS 32
#define PORT 10000

struct epoll_event events[MAX_EPOLL_EVENTS];
struct state state;
int sock;

void handle_sigint(int sig) {
  printf("Terminating");

  event_loop_terminate();
  free_state(&state);
  close(sock);

  exit(0);
}

int main() {
  struct thr_vector *clients = malloc(sizeof(struct thr_vector));
  thr_vector_init(clients);

  struct thr_vector *mm_queue = malloc(sizeof(struct thr_vector));
  thr_vector_init(mm_queue);

  struct thr_vector *ready_checks = malloc(sizeof(struct thr_vector));
  thr_vector_init(ready_checks);

  struct thr_hashmap *games = malloc(sizeof(struct thr_hashmap));
  thr_hashmap_init(games);

  state.clients = clients;
  state.mm_queue = mm_queue;
  state.ready_checks = ready_checks;
  state.games = games;

  event_loop_init();
  mm_loop_init(&state);

  struct sockaddr_in address;
  address.sin_family = AF_UNSPEC;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  sock = socket(AF_INET, SOCK_STREAM, 0);
  int bind_failed = bind(sock, (struct sockaddr *)&address, sizeof(address));

  if (bind_failed) {
    printf("Bind for port %d failed: %d\n", PORT, errno);
    handle_sigint(SIGINT);
    return 1;
  }

  // Set non-blocking
  int sock_flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, sock_flags | O_NONBLOCK);

  listen(sock, 256);

  int epollfd = epoll_create1(0);

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = sock;

  epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev);

  signal(SIGINT, handle_sigint);
  signal(SIGTERM, handle_sigint);
  signal(SIGQUIT, handle_sigint);
  signal(SIGSEGV, handle_sigint);

  int nfds, i;
  int conn_sock, fd;
  int flags;
  while (true) {
    printf("Looping\n");
    nfds = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1);

    for (i = 0; i < nfds; i++) {
      fd = events[i].data.fd;

      if (fd == sock) {
        printf("SOCK EVENT :0\n");

        conn_sock = accept(sock, NULL, NULL);

        // Set non-blocking
        flags = fcntl(conn_sock, F_GETFL, 0);
        fcntl(conn_sock, F_SETFL, flags | O_NONBLOCK);

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = conn_sock;

        epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev);

        struct client *c = malloc(sizeof(struct client));
        c->socket = conn_sock;
        c->player = NULL;
        c->ready_check = NULL;
        c->needs_to_ack_score = NULL;

        thr_vector_push(clients, c);

        printf("Clients now:\n");
        for (int y = 0; y < clients->data->length; y++) {
          struct client *k = clients->data->data[y];
          printf("Client %d\n", k->socket);
        }
        printf("---\n");
        continue;
      }

      // Client event
      struct game_event game_event = event_get(fd);

      struct handle_params *handle_params =
          malloc(sizeof(struct handle_params));

      handle_params->state = &state;
      handle_params->event = game_event;

      struct message *event_message = malloc(sizeof(struct message));
      event_message->callback = &handle;
      event_message->params = handle_params;

      event_loop_add_message(event_message);
    }
  }

  return 0;
}
