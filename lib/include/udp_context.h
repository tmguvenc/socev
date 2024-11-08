#ifndef LIB_UDP_CONTEXT_H_
#define LIB_UDP_CONTEXT_H_

#include "context.h"

void* udp_context_create(const context_params_t* params);
void udp_context_destroy(void* udp_ctx);

int udp_context_service(void* udp_ctx, int timeout_ms);

#endif  // LIB_UDP_CONTEXT_H_
