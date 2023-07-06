#include "tcp_context.h"

#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "client.h"
#include "client_list.h"
#include "epoll_helper.h"
#include "utils.h"

#define INTERNAL_BUFFER_SIZE (64 * 1024)

typedef struct {
  int fd;
  int efd;
  char* recv_buf;
  void (*callback)(const event_type ev, void* client, const void* in,
                   const unsigned int len);
  void* client_list;
  struct epoll_event* events;
} tcp_context;

void* tcp_context_create(tcp_context_params params) {
  tcp_context* ctx = (tcp_context*)calloc(1, sizeof(tcp_context));

  if (!ctx) {
    fprintf(stderr, "tcp_context_create err: cannot create context\n");
    goto create_error;
  }

  ctx->recv_buf = (char*)calloc(1, INTERNAL_BUFFER_SIZE);
  if (!ctx->recv_buf) {
    fprintf(stderr, "tcp_context_create err: %s\n", strerror(errno));
    goto create_error;
  }

  ctx->efd = epoll_create1(0);
  if (ctx->efd == -1) {
    fprintf(stderr, "epoll_create: %s\n", strerror(errno));
    goto create_error;
  }

  ctx->client_list = client_list_create(params.max_client_count);
  if (!ctx->client_list) {
    fprintf(stderr, "tcp_context_create err: cannot create client list\n");
    goto create_error;
  }

  ctx->events =
      calloc(params.max_client_count * 2 + 1, sizeof(struct epoll_event));
  if (!ctx->events) {
    fprintf(stderr, "tcp_context_create err: cannot create event list\n");
    goto create_error;
  }

  ctx->callback = params.callback;

  ctx->fd = create_listener_socket(params.port);
  if (ctx->fd == -1) {
    fprintf(stderr, "socket create failed\n");
    goto create_error;
  }

  if (epoll_ctl_add(ctx->efd, ctx->fd, EPOLLIN) == -1) {
    goto create_error;
  }

  // start listening incoming connections
  if (listen(ctx->fd, params.max_client_count) == -1) {
    fprintf(stderr, "tcp_context_create err: %s\n", strerror(errno));
    goto create_error;
  }

  return ctx;

create_error:
  tcp_context_destroy(ctx);
  return NULL;
}

void tcp_context_destroy(void* tcp_ctx) {
  if (!tcp_ctx) {
    tcp_context* ctx = (tcp_context*)(tcp_ctx);

    // close listening socket
    if (ctx->fd != -1) {
      close(ctx->fd);
    }

    if (ctx->efd != -1) {
      close(ctx->efd);
    }

    // free receive buffer
    if (ctx->recv_buf) {
      free(ctx->recv_buf);
    }

    // free event list
    if (ctx->events) {
      free(ctx->events);
    }

    client_list_destroy(ctx->client_list);

    // release tcp context
    free(ctx);
    ctx = NULL;
  }
}

int do_accept(tcp_context* ctx) {
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t size = sizeof(struct sockaddr_in);

  int new_fd = accept(ctx->fd, (struct sockaddr*)(&client_addr), &size);
  if (new_fd == -1) {
    fprintf(stderr, "do_accept err: %s\n", strerror(errno));
    return -1;
  }

  if (set_socket_nonblocking(new_fd) == -1) {
    return -1;
  }

  void* client = client_create(
      ctx->efd, new_fd, inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

  if (!client) {
    fprintf(stderr, "cannot create new client");
    return -1;
  }

  if (client_list_add_client(ctx->client_list, client) == -1) {
    return -1;
  }

  if (ctx->callback) {
    ctx->callback(EVT_CLIENT_CONNECTED, client, NULL, 0);
  }

  return new_fd;
}

int do_receive(tcp_context* ctx, void* client) {
  int fd = client_get_fd(client);
  ssize_t bytes = recv(fd, ctx->recv_buf, INTERNAL_BUFFER_SIZE, 0);
  if (bytes == -1) {
    fprintf(stderr, "do_receive err: %s\n", strerror(errno));
    return -1;
  }

  // client disconnected
  if (bytes == 0) {
    if (ctx->callback) {
      ctx->callback(EVT_CLIENT_DISCONNECTED, client, NULL, 0);
    }

    return -2;
  }

  // client data received
  if (ctx->callback) {
    ctx->callback(EVT_CLIENT_DATA_RECEIVED, client, ctx->recv_buf, bytes);
  }

  return 0;
}

int tcp_context_service(void* tcp_ctx, int timeout_ms) {
  int nfds, i, fd;
  client_get_result_t get_res;

  if (!tcp_ctx) {
    return -1;
  }

  tcp_context* ctx = (tcp_context*)(tcp_ctx);
  const size_t fd_cnt = client_list_get_max_count(ctx->client_list) * 2 + 1;

  nfds = epoll_wait(ctx->efd, ctx->events, fd_cnt, timeout_ms);

  if (nfds == -1) {
    fprintf(stderr, "socev_service err: %s\n", strerror(errno));
    return nfds;
  }

  for (i = 0; i < nfds; i++) {
    if (ctx->events[i].events & EPOLLIN) {
      fd = ctx->events[i].data.fd;
      if (fd == ctx->fd) {
        // handle incoming connection
        if (do_accept(ctx) == -1) {
          fprintf(stderr, "do_accept failed\n");
        }
        continue;
      }

      if (client_list_get_client(ctx->client_list, fd, &get_res) == -1)
        continue;

      if (get_res.type == FD_TIMER) {
        // process timer expired
        client_set_timer(get_res.client, 0);     // stop the timer
        client_enable_timer(get_res.client, 0);  // disable the timer
        if (ctx->callback) {
          ctx->callback(EVT_CLIENT_TIMER_EXPIRED, get_res.client, NULL, 0);
        }
      }

      // process inbound data
      if (get_res.type == FD_REGULAR) {
        const int recv_res = do_receive(ctx, get_res.client);
        if (recv_res == -1) {
          // handle receive error
          fprintf(stderr, "do_receive failed\n");
        } else if (recv_res == -2) {
          // handle disconnected client
          client_list_del_client(ctx->client_list, fd);
        }
      }
    }
    if (ctx->events[i].events & EPOLLOUT) {
      // process outbound data

      // clear pollout request of the client
      client_clear_callback_on_writable(get_res.client);

      if (ctx->callback) {
        ctx->callback(EVT_CLIENT_WRITABLE, get_res.client, NULL, 0);
      }
    }
  }

  return nfds;
}
