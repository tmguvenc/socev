#include <signal.h>
#include <string.h>

#include "client.h"
#include "stdio.h"
#include "tcp_context.h"

static void callback(const event_type ev, void* client, const void* in,
                     const uint32_t len) {
  const char* client_ip = client_get_ip(client);
  const uint16_t port = client_get_port(client);
  switch (ev) {
    case EVT_CLIENT_CONNECTED:
      printf("client connected: %s:%u\n", client_ip, port);
      break;
    case EVT_CLIENT_DISCONNECTED:
      printf("client disconnected: %s:%d\n", client_ip, port);
      break;
    case EVT_CLIENT_DATA_RECEIVED:
      printf("received from [%s:%d]: %s\n", client_ip, port, (const char*)in);
      client_callback_on_writable(client);
      client_set_timer_us(client, 1000);
      break;
    case EVT_CLIENT_WRITABLE: {
      char buffer[80];
      int pos = snprintf(buffer, sizeof(buffer), "here is your answer\n");
      client_write(client, buffer, pos);
    } break;
    case EVT_CLIENT_TIMER_EXPIRED: {
      char buffer[80];
      int pos = snprintf(buffer, sizeof(buffer), "here is your answer\n");
      client_write(client, buffer, pos);
    } break;
    default:
      break;
  }
}

static int g_interruped;

void signal_handler(int sig) { g_interruped = 1; }

int main(int argc, char* argv[]) {
  signal(SIGINT, signal_handler);

  tcp_context_params params = {
      .port = 9000, .max_client_count = 10, .callback = callback};

  void* ctx = tcp_context_create(params);

  printf("server started\n");

  while (!g_interruped) {
    tcp_context_service(ctx, -1);
  }

  tcp_context_destroy(ctx);

  printf("server stopped\n");

  return 0;
}