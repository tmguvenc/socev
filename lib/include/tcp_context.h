#ifndef LIB_TCP_CONTEXT_H_
#define LIB_TCP_CONTEXT_H_

#include <stdint.h>

typedef enum {
  EVT_CLIENT_CONNECTED = 0,
  EVT_CLIENT_DISCONNECTED,
  EVT_CLIENT_WRITABLE,
  EVT_CLIENT_DATA_RECEIVED,
  EVT_CLIENT_TIMER_EXPIRED,
  __EVT_MAX_COUNT
} event_type;

typedef struct {
  uint16_t port;
  uint64_t max_client_count;
  void (*callback)(const event_type ev, void* c_info, const void* in,
                   const uint32_t len);
} tcp_context_params;

void* tcp_context_create(tcp_context_params params);
void tcp_context_destroy(void* tcp_ctx);

int tcp_context_service(void* tcp_ctx, int timeout_ms);

#endif  // LIB_TCP_CONTEXT_H_
