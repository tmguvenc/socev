#ifndef LIB_TCP_CONTEXT_H_
#define LIB_TCP_CONTEXT_H_

typedef struct {
  unsigned short port;
  unsigned int max_client_count;
} tcp_context_params;

void* socev_create_tcp_context(tcp_context_params params);
void socev_destroy_tcp_context(void* ctx);

int socev_service(void* ctx, int timeout_ms);

#endif // LIB_TCP_CONTEXT_H_
