#ifndef LIB_UTILS_H_
#define LIB_UTILS_H_

#include <stdint.h>

struct pollfd;
struct timespec;

int create_socket(unsigned short port);
int set_socket_nonblocking(int fd);

struct timespec to_timespec(const int64_t interval_us);
int arm_timer(int timer_fd, const int64_t interval_us);

#endif // LIB_UTILS_H_
