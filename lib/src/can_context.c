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

#include "can_message.h"
#include "can_message_list.h"
#include "epoll_helper.h"
#include "result.h"
#include "utils.h"

typedef struct {
  int efd;
  int fd[MAX_BUS_COUNT];
  void* messages;
  uint16_t msg_cnt;
  char recv_buf[sizeof(struct can_frame)];
  callback_f cb;
  struct epoll_event* events;
  uint8_t evt_cnt;
} can_context_t;

void* can_context_create(const can_context_params* params) {
  can_context_t* ctx;
  uint8_t idx;
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

  ctx->messages = can_message_list_create(params->filter_cnt);

  filters =
      (struct can_filter*)calloc(params->filter_cnt, sizeof(struct can_filter));

  for (idx = 0; idx < MAX_BUS_COUNT; ++idx) {
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
      ctx->evt_cnt++;

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
        can_message_list_set_recv_timer(ctx->messages, ctx->msg_cnt, tmp_fd,
                                        ctx->fd[idx]);
        ctx->evt_cnt++;
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
        can_message_list_set_send_timer(ctx->messages, ctx->msg_cnt, tmp_fd,
                                        ctx->fd[idx]);
        ctx->evt_cnt++;
      }

      if (filter->recv_timeout_ms != -1 || filter->send_timeout_ms != -1) {
        ctx->msg_cnt++;
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

  ctx->events = calloc(ctx->evt_cnt, sizeof(struct epoll_event));
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
  uint8_t idx;
  can_context_t* ctx;
  if (!can_ctx) {
    ctx = (can_context_t*)can_ctx;

    can_message_list_destroy(ctx->messages);

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
  ssize_t bytes;
  result_t get_res;

  if (!can_ctx) {
    return -1;
  }

  can_context_t* ctx = (can_context_t*)(can_ctx);
  nfds = epoll_wait(ctx->efd, ctx->events, ctx->evt_cnt, timeout_ms);

  if (nfds == -1) {
    fprintf(stderr, "can service err: %s\n", strerror(errno));
    return nfds;
  }

  for (i = 0; i < nfds; i++) {
    if (ctx->events[i].events & EPOLLIN) {
      fd = ctx->events[i].data.fd;
      can_message_list_get_message(ctx->messages, fd, &get_res);
      switch (get_res.type) {
        case FD_TIMER: {
          if (read(fd, ctx->recv_buf, sizeof(ctx->recv_buf)) == -1) {
            fprintf(stderr, "read failed: [%s]\n", strerror(errno));
          } else {
            disarm_timer(fd);  // stop the timer
            // process timer expired
            if (ctx->cb) {
              ctx->cb(EVT_CLIENT_TIMER_EXPIRED, get_res.client, NULL, 0);
            }
          }
        } break;
        case FD_REGULAR: {
          // process inbound data
          bytes = recv(fd, ctx->recv_buf, sizeof(ctx->recv_buf), 0);
          if (bytes == -1) {
            fprintf(stderr, "do_receive err: %s\n", strerror(errno));
            return -1;
          }

          // client data received
          if (ctx->cb) {
            ctx->cb(EVT_CLIENT_DATA_RECEIVED, get_res.client, ctx->recv_buf,
                    bytes);
          }

        } break;

        default: {
          fprintf(stderr, "invalid fd type (%u)\n", get_res.type);
        } break;
      }
    }
  }

  return nfds;
}
