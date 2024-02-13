#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "debug.h"
#include "event_handlers.h"
#include "event_loop.h"
#include "game.h"
#include "message.h"
#include "mm.h"
#include "net_parser.h"
#include "net_sender.h"
#include "ready_check.h"
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

void end_server() {
  struct thr_vector *clients = &state.clients;

  pthread_mutex_lock(&state.mutex);
  event_loop_terminate();

  for (size_t i = 0; i < clients->data.length; ++i) {
    struct client *client = clients->data.data[i];
    close(client->socket);
    client->socket = -1;
  }

  close(sock);
  pthread_mutex_unlock(&state.mutex);
}

void handle_sigint(int sig) {
  Debug(Main, "Gracefully closing sock from signal\n");

  end_server();
  exit(0);
}

int main() {
  state_init(&state);

  event_loop_init();
  mm_loop_init(&state);

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  sock = socket(AF_INET, SOCK_STREAM, 0);
  int bind_failed = bind(sock, (struct sockaddr *)&address, sizeof(address));

  if (bind_failed) {
    fprintf(stdout, "Bind for port %d failed: %d\n", PORT, errno);
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
    nfds = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1);

    for (i = 0; i < nfds; i++) {
      fd = events[i].data.fd;
      Debug(Main, "Got event on %d\n", fd);

      if (fd == sock) {
        conn_sock = accept(sock, NULL, NULL);

        // Set non-blocking
        flags = fcntl(conn_sock, F_GETFL, 0);
        fcntl(conn_sock, F_SETFL, flags | O_NONBLOCK);

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = conn_sock;

        epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev);

        struct client *c = malloc(sizeof(struct client));
        client_init(c, &state, conn_sock);

        state_push_client(c);

        Debug(Main, "Clients now:\n");
        for (int y = 0; y < state.clients.data.length; y++) {
          struct client *k = state.clients.data.data[y];
          Debug(Main, "Client %d\n", k->socket);
        }
        Debug(Main, "---\n");
        continue;
      }

      struct client *client = state_get_client(&state, fd);
      client->busy = true;

      net_parser_update(client->net_parser, fd);

      struct net_event net_event;

      // Handle 1 event only
      if (net_parser_get_event(client->net_parser, &net_event)) {
        struct message_handler_params *params =
            malloc(sizeof(struct message_handler_params));

        Debug(Main, "Event type %d for client %d\n", net_event.type, fd);

        params->state = &state;
        params->event = net_event;

        struct message *event_message = malloc(sizeof(struct message));
        event_message->params = params;

        if (net_event.type == IN_ERROR_BOGUS) {
          event_message->callback = (void (*)(void *))handle_error;
          event_message->description = "main/event_error";
        } else if (net_event.type == IN_DISCONNECT) {
          event_message->callback = (void (*)(void *))handle_disconnect;
          event_message->description = "main/event_disconnect";
        } else {
          event_message->callback =
              (void (*)(void *))get_handler(net_event.type);
          event_message->description = "main/event_handle";
        }

        event_loop_add_message(event_message);
      }
    }
  }

  Debug(Main, "Gracefully closing sock 1\n");
  end_server();
  return 0;
}
