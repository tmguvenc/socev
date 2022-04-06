#ifndef LIB_CLIENT_INFO_H_
#define LIB_CLIENT_INFO_H_

#include <poll.h>

struct sockaddr_in;

typedef struct ci {
  unsigned short port;
  char ip[16];
  int fd;
  int timer_fd;
  struct ci *next;
  struct pollfd *pfd;
  struct pollfd *timer_pfd;
} client_info;

client_info *client_info_create(client_info **head, int client_fd,
                                struct sockaddr_in *client_addr);

void client_info_delete(client_info **head, client_info *ci);

typedef struct tag {
  unsigned int count;
  struct pollfd fd_list[100];
} pollfd_list;

void add_ci_to_fdlist(pollfd_list **list, client_info *ci);
void del_ci_from_fdlist(pollfd_list **list, client_info *ci);

#endif // LIB_CLIENT_INFO_H_
