#ifndef LIB_TCP_CLIENT_INFO_H_
#define LIB_TCP_CLIENT_INFO_H_

typedef struct {
  unsigned short port;
  char ip[16];
  int fd;
} client_info;

const char *socev_get_connected_client_ip(void *c_info);
unsigned short socev_get_connected_client_port(void *c_info);

#endif // LIB_TCP_CLIENT_INFO_H_
