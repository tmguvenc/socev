#ifndef LIB_CLIENT_LIST_H_
#define LIB_CLIENT_LIST_H_

#include <stdint.h>

typedef enum { FD_REGULAR = 0, FD_TIMER, __MAX_FD_CNT } fd_type_t;

typedef struct {
  fd_type_t type;
  void* client;
} client_get_result_t;

void* client_list_create(uint16_t cnt);
void client_list_destroy(void* cl);
uint16_t client_list_get_count(void* cl);
uint16_t client_list_get_max_count(void* cl);
int client_list_is_full(void* cl);
int client_list_is_empty(void* cl);
int client_list_add_client(void* cl, void* ci);
int client_list_del_client(void* cl, int client_fd);
int client_list_get_client(void* cl, int fd, client_get_result_t* out);

#endif  // LIB_CLIENT_LIST_H_
