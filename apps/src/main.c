#include "stdio.h"
#include "tcp_context.h"
#include <signal.h>
#include <string.h>

void on_client_connected(void *c_info) {
  printf("client connected: %s:%d\n", socev_get_connected_client_ip(c_info),
         socev_get_connected_client_port(c_info));
}

void on_client_disconnected(void *c_info) {
  printf("client disconnected: %s:%d\n", socev_get_connected_client_ip(c_info),
         socev_get_connected_client_port(c_info));
}

void on_received(void *c_info, const void *in, const unsigned int len) {
  printf("received from [%s:%d]: %s\n", socev_get_connected_client_ip(c_info),
         socev_get_connected_client_port(c_info), (const char* )in);
}

static int g_interruped;

void signal_handler(int sig) { g_interruped = 1; }

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);

  tcp_context_params params;
  memset(&params, 0, sizeof(params));

  params.port = 9000;
  params.max_client_count = 10;
  params.on_client_connected = on_client_connected;
  params.on_client_disconnected = on_client_disconnected;
  params.on_received = on_received;

  void *ctx = socev_create_tcp_context(params);

  printf("server started\n");

  while (!g_interruped) {
    socev_service(ctx, -1);
  }

  socev_destroy_tcp_context(ctx);

  printf("server stopped\n");

  return 0;
}