#ifndef LIB_UTILS_H_
#define LIB_UTILS_H_

#include <stdint.h>

int utils_create_listener_socket(uint16_t port);
int utils_set_socket_nonblocking(int fd);

int utils_set_timer_us(int timer_fd, const int64_t interval_us);

#endif  // LIB_UTILS_H_
