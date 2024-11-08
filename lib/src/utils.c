#include "utils.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

int utils_set_socket_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    fprintf(stderr, "get socket flags err: %s\n", strerror(errno));
    return -1;
  }

  flags |= O_NONBLOCK;

  int result = fcntl(fd, F_SETFL, flags);

  if (result == -1) {
    fprintf(stderr, "set socket flags err: %s\n", strerror(errno));
  }

  return result;
}

int utils_create_listener_socket(uint16_t port, const socket_type st) {
  const int optval = 1;
  struct sockaddr_in server = {0};

  int cls = (st == SOCKET_TYPE_TCP) ? SOCK_STREAM : SOCK_DGRAM;

  int socket_fd = socket(AF_INET, cls, 0);
  if (socket_fd == -1) {
    fprintf(stderr, "couldn't create socket: %s\n", strerror(errno));
    goto err;
  }

  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) == -1) {
    fprintf(stderr, "couldn't set socket option: %s\n", strerror(errno));
    goto err;
  }

  if (utils_set_socket_nonblocking(socket_fd) == -1) {
    fprintf(stderr, "couldn't make socket (%d) non-blocking: %s\n", socket_fd,
            strerror(errno));
    goto err;
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = INADDR_ANY;

  if (bind(socket_fd, (struct sockaddr*)(&server),
           sizeof(struct sockaddr_in)) == -1) {
    fprintf(stderr, "couldn't bind socket: %s\n", strerror(errno));
    goto err;
  }

  return socket_fd;

err:
  if (socket_fd != -1) {
    close(socket_fd);
  }

  return -1;
}

static const int64_t kSecToUsec = 1000000;
static const int64_t kUSecToNSec = 1000;

int utils_set_timer_us(int timer_fd, const int64_t interval_us) {
  struct itimerspec new_value = {
      .it_value = {
          .tv_sec = interval_us / kSecToUsec,
          .tv_nsec = (interval_us - (interval_us / kSecToUsec) * kSecToUsec) *
                     kUSecToNSec}};

  if (timerfd_settime(timer_fd, 0, &new_value, NULL) == -1) {
    fprintf(stderr, "timerfd_settime err: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}
