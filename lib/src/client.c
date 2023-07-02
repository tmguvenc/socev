#include "client.h"

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "epoll_helper.h"
#include "utils.h"

typedef struct {
  uint16_t port;
  char ip[16];
  int efd;
  int fd;
  int timer_fd;
  uint32_t events;
  uint32_t timer_events;
} client_t;

void* client_create(int efd, int fd, const char* ip, uint16_t port) {
  client_t* ci = NULL;

  if (efd == -1 || fd == -1) {
    fprintf(stderr, "invalid fd for client\n");
    goto create_err;
  }

  ci = (client_t*)calloc(1, sizeof(client_t));
  if (!ci) {
    fprintf(stderr, "cannot create client\n");
    goto create_err;
  }

  ci->efd = efd;
  ci->fd = fd;
  ci->timer_fd = timerfd_create(CLOCK_REALTIME, 0);
  ci->events = EPOLLIN;
  ci->timer_events = 0;
  if (ci->timer_fd == -1) {
    fprintf(stderr, "cannot create timer for client: %s\n", strerror(errno));
    goto create_err;
  }

  if (epoll_ctl_add(efd, ci->fd, ci->events) == -1) {
    goto create_err;
  }

  if (epoll_ctl_add(efd, ci->timer_fd, ci->timer_events) == -1) {
    goto create_err;
  }

  snprintf(ci->ip, sizeof(ci->ip), "%s", ip);
  ci->port = port;

  return ci;

create_err:
  client_destroy(ci);
  return NULL;
}

void client_destroy(void* client) {
  if (client) {
    client_t* client_info = (client_t*)client;
    if (client_info->fd != -1) {
      epoll_ctl_del(client_info->efd, client_info->fd);
      close(client_info->fd);
    }
    if (client_info->timer_fd != -1) {
      epoll_ctl_del(client_info->efd, client_info->timer_fd);
      close(client_info->timer_fd);
    }

    free(client_info);
  }
}

char* client_get_ip(void* client) {
  if (client) {
    client_t* client_info = (client_t*)client;
    return client_info->ip;
  }

  return NULL;
}

int client_get_fd(void* client) {
  if (client) {
    client_t* client_info = (client_t*)client;
    return client_info->fd;
  }

  return -1;
}

int client_get_timerfd(void* client) {
  if (client) {
    client_t* client_info = (client_t*)client;
    return client_info->timer_fd;
  }

  return -1;
}

uint16_t client_get_port(void* client) {
  if (client) {
    client_t* client_info = (client_t*)client;
    return client_info->port;
  }

  return 0;
}

void client_enable_timer(void* client, int en) {
  if (client) {
    client_t* inf = (client_t*)client;
    if (en) {
      inf->timer_events |= EPOLLIN;
    } else {
      inf->timer_events &= ~EPOLLIN;
    }
    epoll_ctl_change(inf->efd, inf->timer_fd, inf->timer_events);
  }
}

void client_set_timer(void* client, const uint64_t timeout_us) {
  if (client) {
    client_t* inf = (client_t*)client;
    if (timeout_us != 0) {
      arm_timer(inf->timer_fd, timeout_us);
    } else {
      disarm_timer(inf->timer_fd);
    }
  }
}

void client_callback_on_writable(void* client) {
  if (client) {
    client_t* inf = (client_t*)client;
    inf->events |= EPOLLOUT;
    epoll_ctl_change(inf->efd, inf->fd, inf->events);
  }
}

void client_clear_callback_on_writable(void* client) {
  if (client) {
    client_t* inf = (client_t*)client;
    inf->events &= ~EPOLLOUT;
    epoll_ctl_change(inf->efd, inf->fd, inf->events);
  }
}

int client_write(void* client, const void* data, unsigned int len) {
  if (!client) {
    fprintf(stderr, "socev_write err: invalid client info\n");
    return -1;
  }

  int fd = client_get_fd(client);
  int result = write(fd, data, len);

  if (result == -1) {
    fprintf(stderr, "socev_write err: %s\n", strerror(errno));
  }

  return result;
}
