#include "context.h"

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "client.h"
#include "client_list.h"
#include "utils.h"

context_t* context_create(const context_params_t* params) {
  context_t* ctx = NULL;

  if (!params || !params->recv_buf_size || !params->port ||
      params->max_client_count) {
    fprintf(stderr, "invalid context parameters\n");
    goto err;
  }

  ctx = (context_t*)malloc(sizeof(context_t));
  if (!ctx) {
    fprintf(stderr, "cannot create context\n");
    goto err;
  }

  ctx->recv_buf_size = params->recv_buf_size;

  ctx->recv_buf = (char*)calloc(1, ctx->recv_buf_size);
  if (!ctx->recv_buf) {
    fprintf(stderr, "cannot create context buffer: %s\n", strerror(errno));
    goto err;
  }

  ctx->efd = epoll_create1(0);
  if (ctx->efd == -1) {
    fprintf(stderr, "epoll_create: %s\n", strerror(errno));
    goto err;
  }

  ctx->client_list = client_list_create(params->max_client_count);
  if (!ctx->client_list) {
    fprintf(stderr, "cannot create client list\n");
    goto err;
  }

  ctx->events =
      calloc(params->max_client_count * 2 + 1, sizeof(struct epoll_event));
  if (!ctx->events) {
    fprintf(stderr, "cannot create event list\n");
    goto err;
  }

  ctx->callback = params->callback;

  return ctx;

err:
  context_destroy(ctx);
  return NULL;
}

void context_destroy(context_t* ctx) {
  if (ctx) {
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
  }
}
