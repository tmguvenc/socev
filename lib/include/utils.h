#ifndef LIB_UTILS_H_
#define LIB_UTILS_H_

struct pollfd;

int create_socket(unsigned short port);
int set_socket_nonblocking(int fd);

struct pollfd* create_fd_list(unsigned int count);

#endif // LIB_UTILS_H_
