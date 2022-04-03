#include "tcp_context.h"
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct {
  void (*on_client_connected)(void *c_info);
  void (*on_client_disconnected)(void *c_info);
} events;

typedef struct {
  unsigned short port;
  char ip[16];
  int fd;
} client_info;

typedef struct {
  int fd;
  unsigned short port;
  unsigned int max_client_count;
  client_info *client_info_list;
  struct pollfd *fd_list;
  int curr_idx;
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
  memset(ctx->fd_list, 0, (ctx->max_client_count + 1) * sizeof(client_info));

  // 1st pollfd is for the listening fd
  ctx->curr_idx = 0;
  ctx->fd_list[ctx->curr_idx].fd = ctx->fd;
  ctx->fd_list[ctx->curr_idx].events = POLLIN;
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

  if (tcp_ctx->events.on_client_connected) {
    tcp_ctx->events.on_client_connected(c_info);
  }

  tcp_ctx->curr_idx++;

  return new_client_fd;
}

int socev_service(void *ctx, int timeout_ms) {
  int result = -1;

  if (!ctx) {
    tcp_context *tcp_ctx = (tcp_context *)(ctx);
    result = poll(tcp_ctx->fd_list, tcp_ctx->curr_idx, timeout_ms);

    if (result == -1) {
      fprintf(stderr, "socev_service err: %s\n", strerror(errno));
      return result;
    }

    if (result > 0) {
      if (tcp_ctx->fd_list[0].revents & POLLIN) {
        if(do_accept(tcp_ctx) == -1) {
          fprintf(stderr, "do_accept failed\n");
        }
      }

      
    }
  }

  return result;
}