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
    client_info *inf = (client_info *)c_info;
    return inf->ip;
  }
  return NULL;
}

unsigned short socev_get_client_port(void *c_info) {
  if (c_info) {
    client_info *inf = (client_info *)c_info;
    return inf->port;
  }
  return 0;
}

typedef struct {
  int fd;
  unsigned int max_client_count;
  client_info *ci_list;
  pollfd_list *pfd_list;
  char *receive_buffer;
  void (*callback)(const event_type ev, void *c_info, const void *in,
                   const unsigned int len);
} tcp_context;

void *socev_create_tcp_context(tcp_context_params params) {
  tcp_context *ctx = (tcp_context *)calloc(1, sizeof(tcp_context));

  if (!ctx) {
    fprintf(stderr, "socev_create_tcp_context err: cannot create context\n");
    return NULL;
  }

  ctx->receive_buffer = (char *)calloc(1, INTERNAL_BUFFER_SIZE);
  if (!ctx->receive_buffer) {
    fprintf(stderr, "socev_create_tcp_context err: %s\n", strerror(errno));
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  ctx->callback = params.callback;

  ctx->fd = create_socket(params.port);
  if (ctx->fd == -1) {
    fprintf(stderr, "socket create failed\n");
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  ctx->pfd_list = (pollfd_list *)calloc(1, sizeof(pollfd_list));
  ctx->pfd_list->fd_list[ctx->pfd_list->count].fd = ctx->fd;
  ctx->pfd_list->fd_list[ctx->pfd_list->count++].events = POLLIN;

  // start listening incoming connections
  if (listen(ctx->fd, ctx->max_client_count) == -1) {
    fprintf(stderr, "socev_create_tcp_context err: %s\n", strerror(errno));
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  return ctx;
}

void socev_destroy_tcp_context(void *ctx) {
  if (!ctx) {
    tcp_context *tcp_ctx = (tcp_context *)(ctx);

    // release client info list
    if (tcp_ctx->ci_list) {
      // iterate through the client fd list
      for (unsigned int idx = 0; idx < tcp_ctx->max_client_count; ++idx) {
        if (tcp_ctx->ci_list[idx].fd != -1) {
          close(tcp_ctx->ci_list[idx].fd);
        }
        if (tcp_ctx->ci_list[idx].timer_fd != -1) {
          close(tcp_ctx->ci_list[idx].timer_fd);
        }
      }
      free(tcp_ctx->ci_list);
    }

    // close listening socket
    if (tcp_ctx->fd != -1) {
      close(tcp_ctx->fd);
    }

    // free receive buffer
    if (tcp_ctx->receive_buffer) {
      free(tcp_ctx->receive_buffer);
    }

    // free pollfd list
    if (tcp_ctx->pfd_list) {
      free(tcp_ctx->pfd_list);
    }

    // release tcp context
    free(tcp_ctx);
    tcp_ctx = NULL;
  }
}

void socev_set_timer(void *c_info) {
  if (c_info) {
    client_info *inf = (client_info *)c_info;
    inf->timer_pfd->events |= POLLIN;
  }
}

void socev_callback_on_writable(void *c_info) {
  if (c_info) {
    client_info *inf = (client_info *)c_info;
    inf->pfd->events |= POLLOUT;
  }
}

int do_accept(tcp_context *tcp_ctx) {
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t size = sizeof(struct sockaddr_in);

  int new_client_fd =
      accept(tcp_ctx->fd, (struct sockaddr *)(&client_addr), &size);
  if (new_client_fd == -1) {
    fprintf(stderr, "do_accept err: %s\n", strerror(errno));
    return -1;
  }

  if (set_socket_nonblocking(new_client_fd) == -1) {
    return -1;
  }

  client_info *c_info =
      client_info_create(&tcp_ctx->ci_list, new_client_fd, &client_addr);

  add_ci_to_fdlist(&tcp_ctx->pfd_list, c_info);

  if (tcp_ctx->callback) {
    tcp_ctx->callback(CLIENT_CONNECTED, c_info, NULL, 0);
  }

  return new_client_fd;
}

int do_receive(tcp_context *tcp_ctx, client_info *c_info) {
  // clear the internal buffer
  memset(tcp_ctx->receive_buffer, 0, INTERNAL_BUFFER_SIZE);

  ssize_t bytes =
      recv(c_info->fd, tcp_ctx->receive_buffer, INTERNAL_BUFFER_SIZE, 0);
  if (bytes == -1) {
    fprintf(stderr, "do_receive err: %s\n", strerror(errno));
    return -1;
  }

  // client disconnected
  if (bytes == 0) {
    if (tcp_ctx->callback) {
      tcp_ctx->callback(CLIENT_DISCONNECTED, c_info, NULL, 0);
      del_ci_from_fdlist(&tcp_ctx->pfd_list, c_info);
      client_info_delete(&tcp_ctx->ci_list, c_info);
      close(c_info->fd);
      close(c_info->timer_fd);
      memset(c_info, 0, sizeof(client_info));
    }

    return 0;
  }

  // client data received
  if (tcp_ctx->callback) {
    tcp_ctx->callback(CLIENT_DATA_RECEIVED, c_info, tcp_ctx->receive_buffer,
                      bytes);
  }

  return 0;
}

int socev_write(void *c_info, const void *data, unsigned int len) {
  if (!c_info) {
    fprintf(stderr, "socev_write err: invalid client info\n");
    return -1;
  }

  client_info *inf = (client_info *)c_info;
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
    result =
        poll(tcp_ctx->pfd_list->fd_list, tcp_ctx->pfd_list->count, timeout_ms);

    if (result == -1) {
      fprintf(stderr, "socev_service err: %s\n", strerror(errno));
      return result;
    }

    if (result > 0) {
      // handle incoming connection
      if (tcp_ctx->pfd_list->fd_list[0].revents & POLLIN) {
        if (do_accept(tcp_ctx) == -1) {
          fprintf(stderr, "do_accept failed\n");
        }
      }

      // iterate through the connected client list
      for (client_info *c_info = tcp_ctx->ci_list; c_info != NULL;
           c_info = c_info->next) {
        // process outbound data
        if ((c_info->pfd->events & POLLOUT) &&
            (c_info->pfd->revents & POLLOUT)) {
          if (tcp_ctx->callback) {
            tcp_ctx->callback(CLIENT_WRITABLE, c_info, NULL, 0);
          }

          // clear pollout request of the client
          c_info->pfd->events &= ~POLLOUT;
        }

        // process timer expired
        if (c_info->timer_pfd->revents & POLLIN) {
          fprintf(stderr, "timer expired for [%s:%d]\n", c_info->ip,
                  c_info->port);
        }

        // process inbound data
        if (c_info->pfd->revents & POLLIN) {
          if (do_receive(tcp_ctx, c_info) == -1) {
            fprintf(stderr, "do_receive failed\n");
          }
        }
      }
    }
  }

  return result;
}
