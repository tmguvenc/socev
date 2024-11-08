#ifndef LIB_CONTEXT_H_
#define LIB_CONTEXT_H_

#include "common.h"

typedef struct {
  uint16_t port;
  uint16_t recv_buf_size;
  uint64_t max_client_count;
  callback_f callback;
} context_params_t;

typedef struct {
  int efd;
  char* recv_buf;
  uint16_t recv_buf_size;
  callback_f callback;
  void* client_list;
  struct epoll_event* events;
} context_t;

context_t* context_create(const context_params_t* params);
void context_destroy(context_t* tcp_ctx);

#endif  // LIB_CONTEXT_H_
