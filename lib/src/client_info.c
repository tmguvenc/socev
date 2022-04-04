#include "client_info.h"
#include <stddef.h>

const char *socev_get_connected_client_ip(void *c_info) {
  if (c_info) {
    client_info *inf = (client_info *)c_info;
    return inf->ip;
  }
  return NULL;
}

unsigned short socev_get_connected_client_port(void *c_info) {
  if (c_info) {
    client_info *inf = (client_info *)c_info;
    return inf->port;
  }
  return 0;
}