#ifndef LIB_CLIENT_INFO_H_
#define LIB_CLIENT_INFO_H_

#include <poll.h>
#include <stdint.h>

struct sockaddr_in;

typedef struct ci {
  uint16_t port;
  char ip[16];
  int fd;
  int timer_fd;
  struct pollfd *pfd;
  struct pollfd *timer_pfd;
} ci_t;

typedef struct ci_list {
  ci_t *ci_lst;
  struct pollfd *pfd_lst;
  uint32_t count;
} ci_list_t;

ci_list_t *ci_list_create(const uint64_t capacity);
void ci_list_destroy(ci_list_t *ci_list);

ci_t *add_ci(ci_list_t **list, const int fd, const struct sockaddr_in *addr);
int del_ci(ci_list_t **list, const int fd);

#endif // LIB_CLIENT_INFO_H_
