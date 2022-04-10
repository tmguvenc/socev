#include "tcp_context.h"
#include "client_info.h"
#include "utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#define INTERNAL_BUFFER_SIZE (64 * 1024)

const char *socev_get_client_ip(void *c_info) {
  if (c_info) {
    ci_t *inf = (ci_t *)c_info;
    return inf->ip;
  }
  return NULL;
}

unsigned short socev_get_client_port(void *c_info) {
  if (c_info) {
    ci_t *inf = (ci_t *)c_info;
    return inf->port;
  }
  return 0;
}

typedef struct {
  int fd;
  unsigned int max_client_count;
  ci_list_t *ci_list;
  char *recv_buf;
  void (*callback)(const event_type ev, void *c_info, const void *in,
                   const unsigned int len);
} tcp_context;

void *socev_create_tcp_context(tcp_context_params params) {
  tcp_context *ctx = (tcp_context *)calloc(1, sizeof(tcp_context));

  if (!ctx) {
    fprintf(stderr, "socev_create_tcp_context err: cannot create context\n");
    return NULL;
  }

  ctx->recv_buf = (char *)calloc(1, INTERNAL_BUFFER_SIZE);
  if (!ctx->recv_buf) {
    fprintf(stderr, "socev_create_tcp_context err: %s\n", strerror(errno));
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  ctx->callback = params.callback;

  ctx->fd = create_listener_socket(params.port);
  if (ctx->fd == -1) {
    fprintf(stderr, "socket create failed\n");
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  ctx->ci_list = ci_list_create(params.max_client_count);

  ctx->ci_list->pfd_lst[0].fd = ctx->fd;
  ctx->ci_list->pfd_lst[0].events = POLLIN;

  // start listening incoming connections
  if (listen(ctx->fd, ctx->max_client_count) == -1) {
    fprintf(stderr, "socev_create_tcp_context err: %s\n", strerror(errno));
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  return ctx;
}

void socev_destroy_tcp_context(void *tcp_ctx) {
  if (!tcp_ctx) {
    tcp_context *ctx = (tcp_context *)(tcp_ctx);

    // release client info list
    ci_list_destroy(ctx->ci_list);

    // close listening socket
    if (ctx->fd != -1) {
      close(ctx->fd);
    }

    // free receive buffer
    if (ctx->recv_buf) {
      free(ctx->recv_buf);
    }

    // release tcp context
    free(ctx);
    ctx = NULL;
  }
}

void socev_set_timer(void *c_info, const uint64_t timeout_us) {
  if (c_info) {
    ci_t *inf = (ci_t *)c_info;
    inf->timer_pfd->events |= POLLIN;
    arm_timer(inf->timer_fd, timeout_us);
  }
}

void socev_callback_on_writable(void *c_info) {
  if (c_info) {
    ci_t *inf = (ci_t *)c_info;
    inf->pfd->events |= POLLOUT;
  }
}

int do_accept(tcp_context *ctx) {
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t size = sizeof(struct sockaddr_in);

  int new_fd = accept(ctx->fd, (struct sockaddr *)(&client_addr), &size);
  if (new_fd == -1) {
    fprintf(stderr, "do_accept err: %s\n", strerror(errno));
    return -1;
  }

  if (set_socket_nonblocking(new_fd) == -1) {
    return -1;
  }

  ci_t *c_info = add_ci(&ctx->ci_list, new_fd, &client_addr);

  if (ctx->callback) {
    ctx->callback(CLIENT_CONNECTED, c_info, NULL, 0);
  }

  return new_fd;
}

int do_receive(tcp_context *ctx, ci_t *c_info) {
  // clear the internal buffer
  memset(ctx->recv_buf, 0, INTERNAL_BUFFER_SIZE);

  ssize_t bytes = recv(c_info->fd, ctx->recv_buf, INTERNAL_BUFFER_SIZE, 0);
  if (bytes == -1) {
    fprintf(stderr, "do_receive err: %s\n", strerror(errno));
    return -1;
  }

  // client disconnected
  if (bytes == 0) {
    if (ctx->callback) {
      ctx->callback(CLIENT_DISCONNECTED, c_info, NULL, 0);
    }

    return -2;
  }

  // client data received
  if (ctx->callback) {
    ctx->callback(CLIENT_DATA_RECEIVED, c_info, ctx->recv_buf, bytes);
  }

  return 0;
}

int socev_write(void *c_info, const void *data, unsigned int len) {
  if (!c_info) {
    fprintf(stderr, "socev_write err: invalid client info\n");
    return -1;
  }

  ci_t *inf = (ci_t *)c_info;
  int result = write(inf->fd, data, len);

  if (result == -1) {
    fprintf(stderr, "socev_write err: %s\n", strerror(errno));
  }

  return result;
}

int socev_service(void *ctx, int timeout_ms) {
  int result = -1;

  if (ctx) {
    tcp_context *tcp_ctx = (tcp_context *)(ctx);
    const size_t fd_cnt = tcp_ctx->ci_list->count * 2 + 1;

    result = poll(tcp_ctx->ci_list->pfd_lst, fd_cnt, timeout_ms);

    if (result == -1) {
      fprintf(stderr, "socev_service err: %s\n", strerror(errno));
      return result;
    }

    if (result > 0) {
      ci_list_t *ci_list = tcp_ctx->ci_list;
      struct pollfd *pfd_lst = ci_list->pfd_lst;

      // handle incoming connection
      if (pfd_lst[0].revents & POLLIN) {
        if (do_accept(tcp_ctx) == -1) {
          fprintf(stderr, "do_accept failed\n");
        }
      }

      // iterate through the connected client list
      for (size_t i = 0; i < ci_list->count; ++i) {
        ci_t *c_info = &ci_list->ci_lst[i];
        struct pollfd *pfd = &pfd_lst[2 * i + 1];
        struct pollfd *timer_pfd = &pfd_lst[2 * i + 2];

        // process outbound data
        if ((pfd->events & POLLOUT) && (pfd->revents & POLLOUT)) {
          if (tcp_ctx->callback) {
            tcp_ctx->callback(CLIENT_WRITABLE, c_info, NULL, 0);
          }

          // clear pollout request of the client
          pfd->events &= ~POLLOUT;
        }

        // process timer expired
        if (timer_pfd->revents & POLLIN) {
          if (tcp_ctx->callback) {
            tcp_ctx->callback(CLIENT_TIMER_EXPIRED, c_info, NULL, 0);
          }
          timer_pfd->events &= ~POLLIN;
        }

        // process inbound data
        if (pfd->revents & POLLIN) {
          const int recv_res = do_receive(tcp_ctx, c_info);
          if (recv_res == -1) {
            // handle receive error
            fprintf(stderr, "do_receive failed\n");
          } else if (recv_res == -2) {
            // handle disconnected client
            close(c_info->fd);
            close(c_info->timer_fd);
          }
        }
      }
    }
  }

  return result;
}
