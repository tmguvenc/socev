#ifndef LIB_TCP_CONTEXT_H_
#define LIB_TCP_CONTEXT_H_

typedef struct {
  unsigned short port;
  unsigned int max_client_count;
  void (*on_client_connected)(void *c_info);
  void (*on_client_disconnected)(void *c_info);
  void (*on_client_writable)(void *c_info);
  void (*on_received)(void *c_info, const void *in, const unsigned int len);
} tcp_context_params;

void *socev_create_tcp_context(tcp_context_params params);
void socev_destroy_tcp_context(void *ctx);

const char *socev_get_connected_client_ip(void *c_info);
unsigned short socev_get_connected_client_port(void *c_info);

void socev_callback_on_writable(void *c_info);
int socev_service(void *ctx, int timeout_ms);

int socev_write(void *c_info, const void *data, unsigned int len);

#endif // LIB_TCP_CONTEXT_H_
