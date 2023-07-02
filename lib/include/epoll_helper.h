#ifndef LIB_EPOLL_HELPER_H_
#define LIB_EPOLL_HELPER_H_

#include <stdint.h>

int epoll_ctl_add(int epfd, int fd, uint32_t events);
int epoll_ctl_del(int epfd, int fd);
int epoll_ctl_change(int epfd, int fd, uint32_t events);

#endif  // LIB_EPOLL_HELPER_H_
