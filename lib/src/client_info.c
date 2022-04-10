#include "client_info.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>

ci_list_t *ci_list_create(const uint64_t capacity) {
  ci_list_t *ci_list = calloc(1, sizeof(ci_list_t));
  if (!ci_list) {
    fprintf(stderr, "cannot create client info list\n");
    return NULL;
  }

  ci_list->ci_lst = calloc(capacity, sizeof(ci_t));
  if (!ci_list->ci_lst) {
    free(ci_list);
    fprintf(stderr, "cannot create %lu client info(s)\n", capacity);
    return NULL;
  }

  ci_list->pfd_lst = calloc(capacity * 2 + 1, sizeof(struct pollfd));
  if (!ci_list->pfd_lst) {
    free(ci_list);
    free(ci_list->ci_lst);
    fprintf(stderr, "cannot create %lu pollfd(s)\n", capacity * 2 + 1);
    return NULL;
  }

  return ci_list;
}

#define close_fd(fd)                                                           \
  {                                                                            \
    int fd_ = fd;                                                              \
    if (fd_ != 0) {                                                            \
      close(fd_);                                                              \
    }                                                                          \
  }

void ci_list_destroy(ci_list_t *ci_list) {
  if (ci_list) {
    if (ci_list->ci_lst) {
      for (size_t idx = 1; idx < ci_list->count; ++idx) {
        close_fd(ci_list->ci_lst->fd);
        close_fd(ci_list->ci_lst->timer_fd);
      }
      free(ci_list->ci_lst);
    }
    if (ci_list->pfd_lst) {
      free(ci_list->pfd_lst);
    }
    free(ci_list);
  }
}

ci_t *add_ci(ci_list_t **list, const int fd, const struct sockaddr_in *addr) {
  ci_list_t *ci_list = *list;
  if (!ci_list) {
    fprintf(stderr, "Invalid ci list\n");
    return NULL;
  }

  // get the current client idx
  uint64_t idx = ci_list->count++;

  // select the client bucket
  ci_t *ci = &ci_list->ci_lst[idx];

  // select the corresponding pollfd buckets
  struct pollfd *pfd = &ci_list->pfd_lst[idx * 2 + 1];
  struct pollfd *timer_pfd = &ci_list->pfd_lst[idx * 2 + 2];

  // assign the socket and timer file descriptors.
  ci->fd = fd;
  ci->timer_fd = timerfd_create(CLOCK_REALTIME, 0);

  // assign the local port and remote ip address of connected client
  ci->port = addr->sin_port;
  strcpy(ci->ip, inet_ntoa(addr->sin_addr));

  // set the socket pollfd
  pfd->fd = ci->fd;
  pfd->events = POLLIN;

  // set the timer pollfd
  timer_pfd->fd = ci->timer_fd;
  timer_pfd->events = POLLIN;

  ci->pfd = pfd;
  ci->timer_pfd = timer_pfd;

  return ci;
}

int get_idx(const ci_list_t *list, const int fd) {
  for (int idx = 1; idx < list->count; ++idx) {
    ci_t *tmp_ci = &list->ci_lst[idx];
    if (tmp_ci->fd == fd) {
      return idx;
    }
  }
  return -1;
}

void swap_ci(ci_t *ci_1, ci_t *ci_2) {
  ci_t temp;
  memcpy(&temp, ci_1, sizeof(ci_t));
  memcpy(ci_1, ci_2, sizeof(ci_t));
  memcpy(ci_2, &temp, sizeof(ci_t));
}

void swap_pfd(struct pollfd *pfd_1, struct pollfd *pfd_2) {
  struct pollfd temp;
  memcpy(&temp, pfd_1, sizeof(struct pollfd));
  memcpy(pfd_1, pfd_2, sizeof(struct pollfd));
  memcpy(pfd_2, &temp, sizeof(struct pollfd));
}

int del_ci(ci_list_t **list, const int fd) {
  ci_list_t *ci_list = *list;
  if (!ci_list) {
    fprintf(stderr, "Invalid ci list\n");
    return -1;
  }

  int idx = get_idx(ci_list, fd);
  if (idx == -1) {
    fprintf(stderr, "cannot find ci wit socket fd: [%d]\n", fd);
    return -1;
  }

  // select the ci at idx
  ci_t *ci_idx = &ci_list->ci_lst[idx];

  // select the last ci
  ci_t *ci_lst = &ci_list->ci_lst[ci_list->count - 1];

  // select the pollfds at idx
  struct pollfd *pfd_idx = &ci_list->pfd_lst[idx];
  struct pollfd *timer_pfd_idx = &ci_list->pfd_lst[idx + 1];

  // select the last pollfds
  struct pollfd *pfd_lst = &ci_list->pfd_lst[ci_list->count * 2];
  struct pollfd *timer_pfd_lst = &ci_list->pfd_lst[ci_list->count * 2 + 1];

  if (ci_idx != ci_lst) {
    swap_ci(ci_idx, ci_lst);
    swap_pfd(pfd_idx, pfd_lst);
    swap_pfd(timer_pfd_idx, timer_pfd_lst);
  }

  --ci_list->count;

  return 0;
}
