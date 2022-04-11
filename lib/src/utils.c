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

int set_socket_nonblocking(int fd) {
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

int create_listener_socket(uint16_t port) {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    fprintf(stderr, "create_listener_socket err: %s\n", strerror(errno));
    return -1;
  }

  const int optval = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) == -1) {
    fprintf(stderr, "create_listener_socket err: %s\n", strerror(errno));
    close(socket_fd);
    return -1;
  }

  struct sockaddr_in server;
  memset(&server, 0, sizeof(struct sockaddr_in));

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = INADDR_ANY;

  if (bind(socket_fd, (struct sockaddr*)(&server),
           sizeof(struct sockaddr_in)) == -1) {
    fprintf(stderr, "create_listener_socket err: %s\n", strerror(errno));
    close(socket_fd);
    return -1;
  }

  return socket_fd;
}

struct pollfd* create_fd_list(uint32_t count) {
  struct pollfd* fd_list =
      (struct pollfd*)malloc(count * sizeof(struct pollfd));

  if (!fd_list) {
    fprintf(stderr,
            "create_fd_list err: cannot allocate memory for %d pollfd\n",
            count);
    return NULL;
  }

  // clear pollfd list
  memset(fd_list, 0, count * sizeof(struct pollfd));

  return fd_list;
}

struct timespec to_timespec(const int64_t interval_us) {
  struct timespec ts = {
      .tv_sec = interval_us / 1e6,
      .tv_nsec = (interval_us - (interval_us / 1e6) * 1e6) * 1e6};

  return ts;
}

int arm_timer(int timer_fd, const int64_t interval_us) {
  struct timespec now;
  int result = clock_gettime(CLOCK_REALTIME, &now);
  if (result == -1) {
    fprintf(stderr, "clock_gettime err: %s\n", strerror(errno));
    return -1;
  }

  struct timespec ti = to_timespec(interval_us);

  struct itimerspec new_value = {
      // initial expiration time
      .it_value = {.tv_sec = now.tv_sec + ti.tv_sec,
                   .tv_nsec = now.tv_nsec + ti.tv_nsec}};

  if (timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1) {
    fprintf(stderr, "timerfd_settime err: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}
