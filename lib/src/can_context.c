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
  int fd;
  int efd;
  int* timer_fds;
  uint8_t timer_fd_cnt;
  char recv_buf[sizeof(struct can_frame)];
  callback_f cb;
  struct epoll_event* events;
} can_context_t;

void* can_context_create(const can_context_params* params) {
  can_context_t* ctx;
  struct can_filter* filters = NULL;
  uint8_t idx = 0, fd_cnt = 0;
  int fd;

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

  ctx->fd = create_can_socket(params->iface);
  if (ctx->fd == -1) {
    fprintf(stderr, "socket create failed\n");
    goto create_err;
  }

  if (epoll_ctl_add(ctx->efd, ctx->fd, EPOLLIN) == -1) {
    goto create_err;
  }

  if (params->filter_cnt > 0) {
    filters = (struct can_filter*)calloc(params->filter_cnt,
                                         sizeof(struct can_filter));

    ctx->timer_fds = (int*)calloc(params->filter_cnt, sizeof(int));

    for (; idx < params->filter_cnt; ++idx) {
      const can_filter_t* filter = &params->filters[idx];
      filters[idx].can_id = filter->id;
      filters[idx].can_mask = filter->mask;

      if (filter->recv_timeout_ms != -1) {
        ctx->timer_fds[ctx->timer_fd_cnt] = timerfd_create(CLOCK_REALTIME, 0);
        if (epoll_ctl_add(ctx->efd, ctx->timer_fds[ctx->timer_fd_cnt],
                          EPOLLIN) == -1) {
          goto create_err;
        }
        ctx->timer_fd_cnt++;
      }
      if (filter->send_timeout_ms != -1) {
        ctx->timer_fds[ctx->timer_fd_cnt] = timerfd_create(CLOCK_REALTIME, 0);
        if (epoll_ctl_add(ctx->efd, ctx->timer_fds[ctx->timer_fd_cnt],
                          EPOLLIN) == -1) {
          goto create_err;
        }
        ctx->timer_fd_cnt++;
      }
    }

    if (setsockopt(ctx->fd, SOL_CAN_RAW, CAN_RAW_FILTER, filters,
                   sizeof(struct can_filter) * params->filter_cnt) == -1) {
      fprintf(stderr, "cannot add can filters: %s\n", strerror(errno));
      goto create_err;
    }

    free(filters);
  }

  ctx->events = calloc(ctx->timer_fd_cnt + 1, sizeof(struct epoll_event));
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

void can_context_destroy(void* can_ctx) {
  if (!can_ctx) {
    uint8_t idx = 0;
    can_context_t* ctx = (can_context_t*)can_ctx;

    for (; idx > ctx->timer_fd_cnt; ++idx) {
      if (ctx->timer_fds[idx] != -1) {
        close(ctx->timer_fds[idx]);
      }
    }

    if (ctx->timer_fds) {
      free(ctx->timer_fds);
    }

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

int can_context_service(void* can_ctx, int timeout_ms) {
  int nfds, i, fd;
  client_get_result_t get_res;

  if (!can_ctx) {
    return -1;
  }

  can_context_t* ctx = (can_context_t*)(can_ctx);
  const size_t fd_cnt = ctx->timer_fd_cnt + 1;

  nfds = epoll_wait(ctx->efd, ctx->events, fd_cnt, timeout_ms);

  if (nfds == -1) {
    fprintf(stderr, "socev_service err: %s\n", strerror(errno));
    return nfds;
  }

  for (i = 0; i < nfds; i++) {
    if (ctx->events[i].events & EPOLLIN) {
      fd = ctx->events[i].data.fd;

      if (get_res.type == FD_TIMER) {
        // process timer expired
        client_set_timer(get_res.client, 0);     // stop the timer
        client_enable_timer(get_res.client, 0);  // disable the timer
        if (ctx->cb) {
          ctx->cb(EVT_CLIENT_TIMER_EXPIRED, get_res.client, NULL, 0);
        }
      }

      // process inbound data
      if (get_res.type == FD_REGULAR) {
        const int recv_res = do_receive(ctx, get_res.client);
        if (recv_res == -1) {
          // handle receive error
          fprintf(stderr, "do_receive failed\n");
        }
      }
    }
    if (ctx->events[i].events & EPOLLOUT) {
      // process outbound data

      // clear pollout request of the client
      client_clear_callback_on_writable(get_res.client);

      if (ctx->cb) {
        ctx->cb(EVT_CLIENT_WRITABLE, get_res.client, NULL, 0);
      }
    }
  }

  return nfds;
}