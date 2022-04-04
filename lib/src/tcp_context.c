#include "tcp_context.h"
#include "client_info.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define INTERNAL_BUFFER_SIZE (64 * 1024)

typedef struct {
  void (*on_client_connected)(void *c_info);
  void (*on_client_disconnected)(void *c_info);
  void (*on_client_writable)(void *c_info);
  void (*on_received)(void *c_info, const void *in, const unsigned int len);
} events;

typedef struct {
  int fd;
  unsigned short port;
  unsigned int max_client_count;
  client_info *client_info_list;
  struct pollfd *fd_list;
  int curr_idx;
  char *receive_buffer;
  events events;
} tcp_context;

void *socev_create_tcp_context(tcp_context_params params) {
  tcp_context *ctx = (tcp_context *)malloc(sizeof(tcp_context));

  if (!ctx) {
    fprintf(stderr, "socev_create_tcp_context err: cannot create context\n");
    return NULL;
  }

  // clear context
  memset(ctx, 0, sizeof(tcp_context));

  ctx->receive_buffer = (char *)malloc(INTERNAL_BUFFER_SIZE);
  if (!ctx->receive_buffer) {
    fprintf(stderr, "socev_create_tcp_context err: %s\n", strerror(errno));
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  // clear receive buffer
  memset(ctx->receive_buffer, 0, INTERNAL_BUFFER_SIZE);

  ctx->events.on_client_connected = params.on_client_connected;
  ctx->events.on_client_disconnected = params.on_client_disconnected;
  ctx->events.on_received = params.on_received;
  ctx->events.on_client_writable = params.on_client_writable;

  ctx->port = params.port;
  ctx->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (ctx->fd == -1) {
    fprintf(stderr, "socev_create_tcp_context err: %s\n", strerror(errno));
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  const int optval = 1;
  if (setsockopt(ctx->fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) ==
      -1) {
    fprintf(stderr, "socev_create_tcp_context err: %s\n", strerror(errno));
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  struct sockaddr_in server;
  memset(&server, 0, sizeof(struct sockaddr_in));

  server.sin_family = AF_INET;
  server.sin_port = htons(ctx->port);
  server.sin_addr.s_addr = INADDR_ANY;

  if (bind(ctx->fd, (struct sockaddr *)(&server), sizeof(struct sockaddr_in)) ==
      -1) {
    fprintf(stderr, "socev_create_tcp_context err: %s\n", strerror(errno));
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  ctx->max_client_count = params.max_client_count;
  ctx->client_info_list =
      (client_info *)malloc(ctx->max_client_count * sizeof(client_info));
  if (!ctx->client_info_list) {
    fprintf(
        stderr,
        "socev_create_tcp_context err: cannot allocate memory for %d clients\n",
        ctx->max_client_count);
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  // clear client info list
  memset(ctx->client_info_list, 0, ctx->max_client_count * sizeof(client_info));

  // set all client fds to -1
  for (unsigned int idx = 0; idx < ctx->max_client_count; ++idx) {
    ctx->client_info_list[idx].fd = -1;
  }

  ctx->fd_list = (struct pollfd *)malloc(
      (ctx->max_client_count + 1) * // +1 is for the listening fd
      sizeof(struct pollfd));

  if (!ctx->fd_list) {
    fprintf(
        stderr,
        "socev_create_tcp_context err: cannot allocate memory for %d pollfd\n",
        ctx->max_client_count);
    socev_destroy_tcp_context(ctx);
    return NULL;
  }

  // clear pollfd list
  memset(ctx->fd_list, 0, (ctx->max_client_count + 1) * sizeof(struct pollfd));

  // 1st pollfd is for the listening fd
  ctx->curr_idx = 0;
  ctx->fd_list[ctx->curr_idx].fd = ctx->fd;
  ctx->fd_list[ctx->curr_idx].events = POLLIN | POLLOUT | POLLERR | POLLHUP;
  ctx->curr_idx++;

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
    if (tcp_ctx->client_info_list) {
      // iterate through the client fd list
      for (unsigned int idx = 0; idx < tcp_ctx->max_client_count; ++idx) {
        if (tcp_ctx->client_info_list[idx].fd != -1) {
          close(tcp_ctx->client_info_list[idx].fd);
        }
      }
      free(tcp_ctx->client_info_list);
    }

    // release pollfd list
    if (tcp_ctx->fd_list) {
      free(tcp_ctx->fd_list);
    }

    // close listening socket
    if (tcp_ctx->fd != -1) {
      close(tcp_ctx->fd);
    }

    // free receive buffer
    if (tcp_ctx->receive_buffer) {
      free(tcp_ctx->receive_buffer);
    }

    // release tcp context
    free(tcp_ctx);
    tcp_ctx = NULL;
  }
}

int set_socket_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    fprintf(stderr, "get socket flags err: %s\n", strerror(errno));
    return -1;
  }

  flags |= O_NONBLOCK;

  int result = fcntl(fd, F_SETFL, flags);

  if (result == -1) {
    fprintf(stderr, "set socket flags err: %s\n", strerror(errno));
  }

  return result;
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

  client_info *c_info = &tcp_ctx->client_info_list[tcp_ctx->curr_idx];
  c_info->fd = new_client_fd;
  c_info->port = client_addr.sin_port;
  strcpy(c_info->ip, inet_ntoa(client_addr.sin_addr));

  tcp_ctx->fd_list[tcp_ctx->curr_idx].fd = new_client_fd;
  tcp_ctx->fd_list[tcp_ctx->curr_idx].events = POLLIN;
  c_info->pfd = &tcp_ctx->fd_list[tcp_ctx->curr_idx];

  if (tcp_ctx->events.on_client_connected) {
    tcp_ctx->events.on_client_connected(c_info);
  }

  tcp_ctx->curr_idx++;

  return new_client_fd;
}

int do_receive(tcp_context *tcp_ctx, int idx) {
  client_info *c_info = &tcp_ctx->client_info_list[idx];

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
    if (tcp_ctx->events.on_client_disconnected) {
      tcp_ctx->events.on_client_disconnected(c_info);
      memset(c_info, 0, sizeof(client_info));
      tcp_ctx->curr_idx--;
    }

    return 0;
  }

  // client data received
  if (tcp_ctx->events.on_received) {
    tcp_ctx->events.on_received(c_info, tcp_ctx->receive_buffer, bytes);
  }

  return 0;
}

int socev_write(void *c_info, const void *data, unsigned int len) {
  int result;
  if (c_info) {
    client_info *inf = (client_info *)c_info;
    if ((result = write(inf->fd, data, len)) == -1) {
      fprintf(stderr, "socev_write err: %s\n", strerror(errno));
      result = -1;
    }
  }
  return result;
}

int socev_service(void *ctx, int timeout_ms) {
  int result = -1;

  if (ctx) {
    tcp_context *tcp_ctx = (tcp_context *)(ctx);
    result = poll(tcp_ctx->fd_list, tcp_ctx->curr_idx, timeout_ms);

    if (result == -1) {
      fprintf(stderr, "socev_service err: %s\n", strerror(errno));
      return result;
    }

    if (result > 0) {
      for (int idx = 1; idx < tcp_ctx->curr_idx; ++idx) {
        client_info *c_info = &tcp_ctx->client_info_list[idx];
        // process outbound data
        if ((c_info->pfd->events & POLLOUT) &&
            (c_info->pfd->revents & POLLOUT)) {
          if (tcp_ctx->events.on_client_writable) {
            tcp_ctx->events.on_client_writable(c_info);
          }

          // clear pollout request of the client
          c_info->pfd->events &= ~POLLOUT;
        }

        // process inbound data
        if (c_info->pfd->revents & POLLIN) {
          if (do_receive(tcp_ctx, idx) == -1) {
            fprintf(stderr, "do_receive failed\n");
          }
        }
      }

      // handle incoming connection
      if (tcp_ctx->fd_list[0].revents & POLLIN) {
        if (do_accept(tcp_ctx) == -1) {
          fprintf(stderr, "do_accept failed\n");
        }
      }
    }
  }

  return result;
}
