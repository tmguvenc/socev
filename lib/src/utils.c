#include "utils.h"
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
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

int create_socket(unsigned short port) {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    fprintf(stderr, "create_socket err: %s\n", strerror(errno));
    return -1;
  }

  const int optval = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) == -1) {
    fprintf(stderr, "create_socket err: %s\n", strerror(errno));
    close(socket_fd);
    return -1;
  }

  struct sockaddr_in server;
  memset(&server, 0, sizeof(struct sockaddr_in));

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = INADDR_ANY;

  if (bind(socket_fd, (struct sockaddr *)(&server),
           sizeof(struct sockaddr_in)) == -1) {
    fprintf(stderr, "create_socket err: %s\n", strerror(errno));
    close(socket_fd);
    return -1;
  }

  return socket_fd;
}
