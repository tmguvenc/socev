#include "can_context.h"

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "utils.h"
#include "epoll_helper.h"

typedef struct {
  int fd;
  int efd;
  char recv_buf[sizeof(struct can_frame)];
  callback_f cb;
  struct epoll_event* events;
} can_context_t;

void* can_context_create(const can_context_params* params) {
  if (!params) {
    fprintf(stderr, "can_context_create err: invalid parameters\n");
    return NULL;
  }

  can_context_t* ctx = (can_context_t*)calloc(1, sizeof(can_context_t));
  if (!ctx) {
    fprintf(stderr, "can_context_create err: cannot create context\n");
    goto create_err;
  }

  ctx->efd = epoll_create1(0);
  if (ctx->efd == -1) {
    fprintf(stderr, "epoll_create: %s\n", strerror(errno));
    goto create_err;
  }

  ctx->events =
      calloc(params->max_client_count * 2 + 1, sizeof(struct epoll_event));
  if (!ctx->events) {
    fprintf(stderr, "can_context_create err: cannot create event list\n");
    goto create_err;
  }

  ctx->cb = params->cb;

  ctx->fd = create_can_socket(params->iface);
  if (ctx->fd == -1) {
    fprintf(stderr, "socket create failed\n");
    goto create_err;
  }

  if (epoll_ctl_add(ctx->efd, ctx->fd, EPOLLIN) == -1) {
    goto create_err;
  }

create_err:
  can_context_destroy(ctx);
  return NULL;
}

void can_context_destroy(void* can_ctx) {
  if (!can_ctx) {
    can_context_t* ctx = (can_context_t*)(can_ctx);

    // close listening socket
    if (ctx->fd != -1) {
      close(ctx->fd);
    }

    if (ctx->efd != -1) {
      close(ctx->efd);
    }

    // free event list
    if (ctx->events) {
      free(ctx->events);
    }

    // release tcp context
    free(ctx);
    ctx = NULL;
  }
}

int can_context_service(void* can_ctx, int timeout_ms) {}