#include "epoll_helper.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>

int epoll_ctl_add(int epfd, int fd, uint32_t events) {
  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    fprintf(stderr, "epoll_ctl error: [%s]\n", strerror(errno));
    return -1;
  }

  return 0;
}

int epoll_ctl_del(int epfd, int fd) {
  if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
    fprintf(stderr, "epoll_ctl error: [%s]\n", strerror(errno));
    return -1;
  }

  return 0;
}

int epoll_ctl_change(int epfd, int fd, uint32_t events) {
  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;
  if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
    fprintf(stderr, "epoll_ctl error: [%s]\n", strerror(errno));
    return -1;
  }

  return 0;
}
