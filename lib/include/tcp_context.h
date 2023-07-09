#ifndef LIB_TCP_CONTEXT_H_
#define LIB_TCP_CONTEXT_H_

#include "definitions.h"

typedef struct {
  uint16_t port;
  uint64_t max_client_count;
  callback_f cb;
} tcp_context_params;

void* tcp_context_create(const tcp_context_params* params);
void tcp_context_destroy(void* tcp_ctx);

int tcp_context_service(void* tcp_ctx, int timeout_ms);

#endif  // LIB_TCP_CONTEXT_H_
