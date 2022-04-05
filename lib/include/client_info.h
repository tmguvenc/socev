#ifndef LIB_CLIENT_INFO_H_
#define LIB_CLIENT_INFO_H_

struct pollfd;

typedef struct {
  unsigned short port;
  char ip[16];
  int fd;
  int timer_fd;
  struct pollfd *pfd;
  struct pollfd *timer_pfd;
} client_info;

typedef struct {
  client_info* cc_list;
  unsigned int count;
  unsigned int used_idx;
} client_info_list;

client_info_list* client_info_list_create(unsigned int count);
void client_info_list_destroy(client_info_list* list);

#endif // LIB_CLIENT_INFO_H_
