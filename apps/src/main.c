#include "stdio.h"
#include "tcp_context.h"
#include <signal.h>
#include <string.h>

static char buffer[80];

static void callback(const event_type ev, void *c_info, const void *in,
                     const unsigned int len) {
  const char *client_ip = socev_get_client_ip(c_info);
  const unsigned short port = socev_get_client_port(c_info);
  switch (ev) {
  case CLIENT_CONNECTED:
    printf("client connected: %s:%d\n", client_ip, port);
    break;
  case CLIENT_DISCONNECTED:
    printf("client disconnected: %s:%d\n", client_ip, port);
    break;
  case CLIENT_DATA_RECEIVED:
    printf("received from [%s:%d]: %s\n", client_ip, port, (const char *)in);
    socev_callback_on_writable(c_info);
    socev_set_timer(c_info, 1000000);
    break;
  case CLIENT_WRITABLE: {
    memset(buffer, 0, sizeof(buffer));
    int pos = sprintf(buffer, "here is your answer\n");
    socev_write(c_info, buffer, pos);
  } break;
  case CLIENT_TIMER_EXPIRED: {
    memset(buffer, 0, sizeof(buffer));
    int pos = sprintf(buffer, "your time is up\n");
    socev_write(c_info, buffer, pos);
  } break;
  default:
    break;
  }
}

static int g_interruped;

void signal_handler(int sig) { g_interruped = 1; }

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);

  tcp_context_params params = {
      .port = 9000, .max_client_count = 10, .callback = callback};

  void *ctx = socev_create_tcp_context(params);

  printf("server started\n");

  while (!g_interruped) {
    socev_service(ctx, -1);
  }

  socev_destroy_tcp_context(ctx);

  printf("server stopped\n");

  return 0;
}