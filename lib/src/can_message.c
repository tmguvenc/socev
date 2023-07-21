#include "can_message.h"

#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "can_filter.h"
#include "epoll_helper.h"
#include "utils.h"

typedef struct {
  // can_filter_t filter;
  int parent_fd;
  int recv_timer_fd;
  int send_timer_fd;
  int recv_timeout_ms;
  int send_timeout_ms;
} can_message_t;

void* can_message_create(int efd, int parent_fd, int recv_timeout_ms,
                         int send_timeout_ms) {
  can_message_t* msg = (can_message_t*)calloc(1, sizeof(can_message_t));
  if (!msg) {
    fprintf(stderr, "cannot create can message\n");
    return NULL;
  }

  msg->recv_timeout_ms = recv_timeout_ms;
  msg->recv_timeout_ms = send_timeout_ms;
  msg->parent_fd = parent_fd;

  if (recv_timeout_ms != -1) {
    if ((msg->recv_timer_fd = timerfd_create(CLOCK_REALTIME, 0)) == -1) {
      fprintf(stderr, "cannot create timer fd: [%s]\n", strerror(errno));
      goto create_err;
    }

    if (epoll_ctl_add(efd, msg->recv_timer_fd, EPOLLIN) == -1) {
      fprintf(stderr, "cannot add fd [%d] to epoll: [%s]\n", msg->recv_timer_fd,
              strerror(errno));
      goto create_err;
    }
  }

  if (send_timeout_ms != -1) {
    if ((msg->send_timer_fd = timerfd_create(CLOCK_REALTIME, 0)) == -1) {
      fprintf(stderr, "cannot create timer fd: [%s]\n", strerror(errno));
      goto create_err;
    }

    if (epoll_ctl_add(efd, msg->send_timer_fd, EPOLLIN) == -1) {
      fprintf(stderr, "cannot add fd [%d] to epoll: [%s]\n", msg->send_timer_fd,
              strerror(errno));
      goto create_err;
    }
  }

  return msg;

create_err:
  can_message_destroy(msg);
  return NULL;
}

void can_message_destroy(void* msg) {
  if (msg) {
    can_message_t* cmsg = (can_message_t*)msg;
    if (cmsg->recv_timer_fd != -1) {
      close(cmsg->recv_timer_fd);
    }
    if (cmsg->send_timer_fd != -1) {
      close(cmsg->send_timer_fd);
    }

    free(cmsg);
  }
}

int can_message_start_recv_timer(const void* msg) {
  if (msg) {
    can_message_t* cmsg = (can_message_t*)msg;
    return arm_timer(cmsg->recv_timer_fd, cmsg->recv_timeout_ms);
  }
  return -1;
}

int can_message_stop_recv_timer(const void* msg) {
  if (msg) {
    can_message_t* cmsg = (can_message_t*)msg;
    return disarm_timer(cmsg->recv_timer_fd);
  }
  return -1;
}

int can_message_start_send_timer(const void* msg) {
  if (msg) {
    can_message_t* cmsg = (can_message_t*)msg;
    return arm_timer(cmsg->send_timer_fd, cmsg->send_timeout_ms);
  }
  return -1;
}

int can_message_stop_send_timer(const void* msg) {
  if (msg) {
    can_message_t* cmsg = (can_message_t*)msg;
    return disarm_timer(cmsg->send_timer_fd);
  }
  return -1;
}

int can_message_recv_timeout(const void* msg) {
  if (msg) {
    const can_message_t* cmsg = (const can_message_t*)msg;
    return cmsg->recv_timeout_ms;
  }

  return __UINT32_MAX__;
}

int can_message_send_timeout(const void* msg) {
  if (msg) {
    const can_message_t* cmsg = (const can_message_t*)msg;
    return cmsg->send_timeout_ms;
  }

  return __UINT32_MAX__;
}

int can_message_parent_fd(const void* msg) {
  if (msg) {
    const can_message_t* cmsg = (const can_message_t*)msg;
    return cmsg->parent_fd;
  }

  return -1;
}

int can_message_recv_timer_fd(const void* msg) {
  if (msg) {
    const can_message_t* cmsg = (const can_message_t*)msg;
    return cmsg->recv_timer_fd;
  }

  return -1;
}

int can_message_send_timer_fd(const void* msg) {
  if (msg) {
    const can_message_t* cmsg = (const can_message_t*)msg;
    return cmsg->send_timer_fd;
  }

  return -1;
}

uint8_t can_message_active_fd_cnt(const void* msg) {
  if (msg) {
    const can_message_t* cmsg = (const can_message_t*)msg;
    return (cmsg->recv_timer_fd != -1) + (cmsg->send_timer_fd != -1);
  }

  return 0;
}
