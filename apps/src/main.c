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
         socev_get_connected_client_port(c_info), (const char *)in);
  socev_callback_on_writable(c_info);
}

void on_client_writable(void *c_info) {
  static char buffer[80];
  memset(buffer, 0, sizeof(buffer));

  int pos = sprintf(buffer, "answer to [%s:%d]",
                    socev_get_connected_client_ip(c_info),
                    socev_get_connected_client_port(c_info));
  socev_write(c_info, buffer, pos);
}

static int g_interruped;

void signal_handler(int sig) { g_interruped = 1; }

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);

  tcp_context_params params = {
      .port = 9000,
      .max_client_count = 10,
      .on_client_connected = on_client_connected,
      .on_client_disconnected = on_client_disconnected,
      .on_received = on_received,
      .on_client_writable = on_client_writable,
  };

  void *ctx = socev_create_tcp_context(params);

  printf("server started\n");

  while (!g_interruped) {
    socev_service(ctx, -1);
  }

  socev_destroy_tcp_context(ctx);

  printf("server stopped\n");

  return 0;
}