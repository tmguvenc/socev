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

typedef struct {
  int fd;
  context_t* inner_ctx;
} tcp_context;

void* tcp_context_create(const context_params_t* params) {
  tcp_context* ctx = NULL;

  if (!params || !params->recv_buf_size || !params->port ||
      params->max_client_count) {
    fprintf(stderr, "invalid context parameters\n");
    goto err;
  }

  ctx = (tcp_context*)malloc(sizeof(tcp_context));
  if (!ctx) {
    fprintf(stderr, "cannot create tcp context\n");
    goto err;
  }

  ctx->inner_ctx = context_create(params);
  if (!ctx->inner_ctx) {
    fprintf(stderr, "cannot create inner context\n");
    goto err;
  }

  ctx->fd = utils_create_listener_socket(params->port, SOCKET_TYPE_TCP);
  if (ctx->fd == -1) {
    fprintf(stderr, "socket create failed\n");
    goto err;
  }

  if (epoll_ctl_add(ctx->inner_ctx->efd, ctx->fd, EPOLLIN) == -1) {
    goto err;
  }

  // start listening incoming connections
  if (listen(ctx->fd, params->max_client_count) == -1) {
    fprintf(stderr, "tcp_context_create err: %s\n", strerror(errno));
    goto err;
  }

  return ctx;

err:
  tcp_context_destroy(ctx);
  return NULL;
}

void tcp_context_destroy(void* tcp_ctx) {
  if (!tcp_ctx) {
    tcp_context* ctx = (tcp_context*)(tcp_ctx);

    context_destroy(ctx->inner_ctx);

    // close listening socket
    if (ctx->fd != -1) {
      close(ctx->fd);
    }

    // release tcp context
    free(ctx);
    ctx = NULL;
  }
}

static int do_accept(tcp_context* ctx) {
  struct sockaddr_in client_addr = {0};
  socklen_t size = sizeof(struct sockaddr_in);
  void* client = NULL;

  int new_fd = accept(ctx->fd, (struct sockaddr*)(&client_addr), &size);
  if (new_fd == -1) {
    fprintf(stderr, "do_accept err: %s\n", strerror(errno));
    goto err;
  }

  if (utils_set_socket_nonblocking(new_fd) == -1) {
    goto err;
  }

  client = client_create(ctx->inner_ctx->efd, new_fd,
                         inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

  if (!client) {
    fprintf(stderr, "cannot create new client");
    goto err;
  }

  if (client_list_add_client(ctx->inner_ctx->client_list, client) == -1) {
    goto err;
  }

  if (ctx->inner_ctx->callback) {
    ctx->inner_ctx->callback(EVT_CLIENT_CONNECTED, client, NULL, 0);
  }

  return new_fd;

err:
  if (new_fd != -1) {
    close(new_fd);
  }

  client_destroy(client);
  return -1;
}

static int do_receive(context_t* ctx, void* client) {
  const int fd = client_get_fd(client);
  const ssize_t bytes = recv(fd, ctx->recv_buf, ctx->recv_buf_size, 0);
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
  const size_t fd_cnt =
      client_list_get_max_count(ctx->inner_ctx->client_list) * 2 + 1;

  nfds = epoll_wait(ctx->inner_ctx->efd, ctx->inner_ctx->events, fd_cnt,
                    timeout_ms);

  if (nfds == -1) {
    fprintf(stderr, "socev_service err: %s\n", strerror(errno));
    return nfds;
  }

  for (i = 0; i < nfds; i++) {
    if (ctx->inner_ctx->events[i].events & EPOLLIN) {
      fd = ctx->inner_ctx->events[i].data.fd;
      if (fd == ctx->fd) {
        // handle incoming connection
        if (do_accept(ctx) == -1) {
          fprintf(stderr, "do_accept failed\n");
        }
        continue;
      }

      if (client_list_get_client(ctx->inner_ctx->client_list, fd, &get_res) ==
          -1)
        continue;

      if (get_res.type == FD_TIMER) {
        // process timer expired
        client_set_timer_us(get_res.client, 0);

        if (ctx->inner_ctx->callback) {
          ctx->inner_ctx->callback(EVT_CLIENT_TIMER_EXPIRED, get_res.client,
                                   NULL, 0);
        }
      }

      // process inbound data
      if (get_res.type == FD_REGULAR) {
        const int recv_res = do_receive(ctx->inner_ctx, get_res.client);
        if (recv_res == -1) {
          // handle receive error
          fprintf(stderr, "do_receive failed\n");
        } else if (recv_res == -2) {
          // handle disconnected client
          client_list_del_client(ctx->inner_ctx->client_list, fd);
        }
      }
    }
    if (ctx->inner_ctx->events[i].events & EPOLLOUT) {
      // process outbound data

      // clear pollout request of the client
      client_clear_callback_on_writable(get_res.client);

      if (ctx->inner_ctx->callback) {
        ctx->inner_ctx->callback(EVT_CLIENT_WRITABLE, get_res.client, NULL, 0);
      }
    }
  }

  return nfds;
}
