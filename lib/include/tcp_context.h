#ifndef LIB_TCP_CONTEXT_H_
#define LIB_TCP_CONTEXT_H_

#include "context.h"

void* tcp_context_create(const context_params_t* params);
void tcp_context_destroy(void* tcp_ctx);
int tcp_context_service(void* tcp_ctx, int timeout_ms);

#endif  // LIB_TCP_CONTEXT_H_
