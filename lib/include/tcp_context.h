#ifndef LIB_TCP_CONTEXT_H_
#define LIB_TCP_CONTEXT_H_

#include <stdint.h>

typedef enum {
  CLIENT_CONNECTED = 0,
  CLIENT_DISCONNECTED = 1,
  CLIENT_WRITABLE = 2,
  CLIENT_DATA_RECEIVED = 3,
  CLIENT_TIMER_EXPIRED = 4,
} event_type;

typedef struct {
  unsigned short port;
  unsigned int max_client_count;
  void (*callback)(const event_type ev, void *c_info, const void *in,
                   const unsigned int len);
} tcp_context_params;

void *socev_create_tcp_context(tcp_context_params params);
void socev_destroy_tcp_context(void *ctx);

const char *socev_get_client_ip(void *c_info);
unsigned short socev_get_client_port(void *c_info);

void socev_callback_on_writable(void *c_info);
int socev_service(void *ctx, int timeout_ms);

void socev_set_timer(void *c_info, const uint64_t timeout_us);

int socev_write(void *c_info, const void *data, unsigned int len);

#endif // LIB_TCP_CONTEXT_H_
