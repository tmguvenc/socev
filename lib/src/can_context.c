#include "can_context.h"

#include <errno.h>
#include <linux/can/raw.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "epoll_helper.h"
#include "utils.h"

typedef struct {
  can_filter_t filter;
  int parent_fd;
  int recv_timer_id;
  int send_timer_id;
} can_message_t;

typedef struct {
  int efd;
  int fd[MAX_BUS_COUNT];
  can_message_t* messages;
  uint8_t timer_cnt;
  char recv_buf[sizeof(struct can_frame)];
  callback_f cb;
  struct epoll_event* events;
} can_context_t;

static int init_message(can_message_t* msg);

void* can_context_create(const can_context_params* params) {
  can_context_t* ctx;
  uint8_t idx = 0, fd_cnt = 0, cnt;
  int tmp_fd;
  const can_filter_t* filter;
  struct can_filter* filters = NULL;
  uint8_t j, k;

  if (!params) {
    fprintf(stderr, "can_context_create err: invalid can context parameters\n");
    return NULL;
  }

  if (params->filter_cnt != 0 && params->filters == NULL) {
    fprintf(stderr, "can_context_create err: inconsistent filter parameter\n");
    return NULL;
  }

  ctx = (can_context_t*)calloc(1, sizeof(can_context_t));
  if (!ctx) {
    fprintf(stderr, "can_context_create err: cannot create context\n");
    goto create_err;
  }

  ctx->efd = epoll_create1(0);
  if (ctx->efd == -1) {
    fprintf(stderr, "epoll_create: %s\n", strerror(errno));
    goto create_err;
  }

  ctx->cb = params->cb;

  ctx->messages =
      (can_message_t*)calloc(params->filter_cnt, sizeof(can_message_t));

  filters =
      (struct can_filter*)calloc(params->filter_cnt, sizeof(struct can_filter));

  for (idx = 0; idx < MAX_BUS_COUNT; ++idx) {
    init_message(&ctx->messages[idx]);
    ctx->fd[idx] = -1;

    if (strlen(params->ifaces[idx].name) > 0) {
      filter = &params->filters[idx];

      tmp_fd = create_can_socket(params->ifaces[idx].name);
      if (tmp_fd == -1) {
        goto create_err;
      }

      if (epoll_ctl_add(ctx->efd, tmp_fd, EPOLLIN) == -1) {
        goto create_err;
      }

      ctx->fd[idx] = tmp_fd;

      if (filter->recv_timeout_ms != -1) {
        if ((tmp_fd = timerfd_create(CLOCK_REALTIME, 0)) == -1) {
          fprintf(stderr, "cannot create timer fd: [%s]\n", strerror(errno));
          goto create_err;
        }

        if (epoll_ctl_add(ctx->efd, tmp_fd, EPOLLIN) == -1) {
          fprintf(stderr, "cannot add fd [%d] to epoll: [%s]\n", tmp_fd,
                  strerror(errno));
          goto create_err;
        }
        ctx->messages[cnt].recv_timer_id = tmp_fd;
        ctx->messages[cnt].parent_fd = ctx->fd[idx];
      }

      if (filter->send_timeout_ms != -1) {
        if ((tmp_fd = timerfd_create(CLOCK_REALTIME, 0)) == -1) {
          fprintf(stderr, "cannot create timer fd: [%s]\n", strerror(errno));
          goto create_err;
        }

        if (epoll_ctl_add(ctx->efd, tmp_fd, EPOLLIN) == -1) {
          fprintf(stderr, "cannot add fd [%d] to epoll: [%s]\n", tmp_fd,
                  strerror(errno));
          goto create_err;
        }
        ctx->messages[cnt].send_timer_id = tmp_fd;
        ctx->messages[cnt].parent_fd = ctx->fd[idx];
      }

      if (filter->recv_timeout_ms != -1 || filter->send_timeout_ms != -1) {
        cnt++;
      }

      for (k = 0, j = 0; j < params->filter_cnt; ++j) {
        if (strcmp(params->filters[j].iface, filter->iface) == 0) {
          filters[k].can_id = params->filters[j].id;
          filters[k].can_mask = params->filters[j].mask;
          k++;
        }
      }

      if (setsockopt(ctx->fd[idx], SOL_CAN_RAW, CAN_RAW_FILTER, filters,
                     sizeof(struct can_filter) * k) == -1) {
        fprintf(stderr, "cannot add can filters: %s\n", strerror(errno));
        goto create_err;
      }
    }
  }

  free(filters);

  ctx->events = calloc(ctx->timer_cnt + 1, sizeof(struct epoll_event));
  if (!ctx->events) {
    fprintf(stderr, "can_context_create err: cannot create event list\n");
    goto create_err;
  }

create_err:
  if (filters) {
    free(filters);
  }
  can_context_destroy(ctx);
  return NULL;
}

static int init_message(can_message_t* msg) {
  msg->filter.iface = NULL;
  msg->filter.id = 0;
  msg->filter.mask = 0;
  msg->filter.recv_timeout_ms = -1;
  msg->filter.recv_timeout_ms = -1;
  msg->parent_fd = -1;
  msg->recv_timer_id = -1;
  msg->send_timer_id = -1;
}

void can_context_destroy(void* can_ctx) {
  if (!can_ctx) {
    uint8_t idx = 0;
    can_context_t* ctx = (can_context_t*)can_ctx;

    for (; idx > ctx->timer_cnt; ++idx) {
      if (ctx->messages[idx].recv_timer_id != -1) {
        close(ctx->messages[idx].recv_timer_id);
      }
      if (ctx->messages[idx].send_timer_id != -1) {
        close(ctx->messages[idx].send_timer_id);
      }
    }

    if (ctx->messages) {
      free(ctx->messages);
    }

    for (idx = 0; idx < MAX_BUS_COUNT; ++idx) {
      // close socket
      if (ctx->fd[idx] != -1) {
        close(ctx->fd[idx]);
      }
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

int can_context_service(void* can_ctx, int timeout_ms) {
  int nfds, i, fd;
  // client_get_result_t get_res;

  // if (!can_ctx) {
  //   return -1;
  // }

  // can_context_t* ctx = (can_context_t*)(can_ctx);
  // const size_t fd_cnt = ctx->timer_cnt + 1;

  // nfds = epoll_wait(ctx->efd, ctx->events, fd_cnt, timeout_ms);

  // if (nfds == -1) {
  //   fprintf(stderr, "socev_service err: %s\n", strerror(errno));
  //   return nfds;
  // }

  // for (i = 0; i < nfds; i++) {
  //   if (ctx->events[i].events & EPOLLIN) {
  //     fd = ctx->events[i].data.fd;

  //     if (get_res.type == FD_TIMER) {
  //       // process timer expired
  //       client_set_timer(get_res.client, 0);     // stop the timer
  //       client_enable_timer(get_res.client, 0);  // disable the timer
  //       if (ctx->cb) {
  //         ctx->cb(EVT_CLIENT_TIMER_EXPIRED, get_res.client, NULL, 0);
  //       }
  //     }

  //     // process inbound data
  //     if (get_res.type == FD_REGULAR) {
  //       const int recv_res = do_receive(ctx, get_res.client);
  //       if (recv_res == -1) {
  //         // handle receive error
  //         fprintf(stderr, "do_receive failed\n");
  //       }
  //     }
  //   }
  //   if (ctx->events[i].events & EPOLLOUT) {
  //     // process outbound data

  //     // clear pollout request of the client
  //     client_clear_callback_on_writable(get_res.client);

  //     if (ctx->cb) {
  //       ctx->cb(EVT_CLIENT_WRITABLE, get_res.client, NULL, 0);
  //     }
  //   }
  // }

  return nfds;
}