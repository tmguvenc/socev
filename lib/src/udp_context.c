#include "udp_context.h"

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
  context_t* inner;
} udp_context_t;

void* udp_context_create(const context_params_t* params) {
  udp_context_t* ctx = NULL;

  if (!params || !params->recv_buf_size || !params->port ||
      params->max_client_count) {
    fprintf(stderr, "invalid context parameters\n");
    goto err;
  }

  ctx = (udp_context_t*)malloc(sizeof(udp_context_t));
  if (!ctx) {
    fprintf(stderr, "cannot create udp context\n");
    goto err;
  }

  ctx->inner = context_create(params);
  if (!ctx->inner) {
    fprintf(stderr, "cannot create inner context\n");
    goto err;
  }

  ctx->fd = utils_create_listener_socket(params->port, SOCKET_TYPE_UDP);
  if (ctx->fd == -1) {
    fprintf(stderr, "socket create failed\n");
    goto err;
  }

  if (epoll_ctl_add(ctx->inner->efd, ctx->fd, EPOLLIN) == -1) {
    goto err;
  }

  return ctx;

err:
  udp_context_destroy(ctx);
  return NULL;
}

void udp_context_destroy(void* udp_ctx) {
  if (udp_ctx) {
    udp_context_t* ctx = (udp_context_t*)(udp_ctx);

    context_destroy(ctx->inner);

    // close listening socket
    if (ctx->fd != -1) {
      close(ctx->fd);
    }

    // release tcp context
    free(ctx);
    ctx = NULL;
  }
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

int udp_context_service(void* udp_ctx, int timeout_ms) {
  int nfds, i, fd;
  client_get_result_t get_res;

  if (!udp_ctx) {
    return -1;
  }

  udp_context_t* ctx = (udp_context_t*)(udp_ctx);
  context_t* inner_ctx = ctx->inner;
  const size_t fd_cnt = client_list_get_max_count(inner_ctx->client_list) * 2 + 1;

  nfds = epoll_wait(inner_ctx->efd, inner_ctx->events, fd_cnt, timeout_ms);

  if (nfds == -1) {
    fprintf(stderr, "socev_service err: %s\n", strerror(errno));
    return nfds;
  }

  for (i = 0; i < nfds; i++) {
    if (inner_ctx->events[i].events & EPOLLIN) {
      fd = inner_ctx->events[i].data.fd;
      if (fd == ctx->fd) {
        // handle incoming connection
        if (do_accept(ctx) == -1) {
          fprintf(stderr, "do_accept failed\n");
        }
        continue;
      }

      if (client_list_get_client(inner_ctx->client_list, fd, &get_res) == -1)
        continue;

      if (get_res.type == FD_TIMER) {
        // process timer expired
        client_set_timer_us(get_res.client, 0);

        if (inner_ctx->callback) {
          inner_ctx->callback(EVT_CLIENT_TIMER_EXPIRED, get_res.client, NULL, 0);
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
          client_list_del_client(inner_ctx->client_list, fd);
        }
      }
    }
    if (inner_ctx->events[i].events & EPOLLOUT) {
      // process outbound data

      // clear pollout request of the client
      client_clear_callback_on_writable(get_res.client);

      if (inner_ctx->callback) {
        inner_ctx->callback(EVT_CLIENT_WRITABLE, get_res.client, NULL, 0);
      }
    }
  }

  return nfds;
}
