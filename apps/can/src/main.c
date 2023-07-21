#include <linux/can.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "can_context.h"
#include "can_message.h"

static char buffer[80];
static const char* bus0 = "vcan0";
static const char* bus1 = "vcan1";

static void callback(const event_type ev, void* msg, const void* in,
                     const uint32_t len) {
  uint32_t idx;
  struct can_frame cf;
  memcpy(&cf, in, len);
  switch (ev) {
    case EVT_CLIENT_DATA_RECEIVED:
      fprintf(stdout, "received from [0x%08X]: ", cf.can_id);
      for (idx = 0; idx < sizeof(cf.data); idx++) {
        fprintf(stdout, "%02X ", cf.data[idx]);
      }
      fprintf(stdout, "\n");
      can_message_start_recv_timer(msg);
      break;
    case EVT_CLIENT_TIMER_EXPIRED:
      fprintf(stderr, "timer expired for [0x%08X]\n", cf.can_id);
      break;
    default:
      break;
  }
}

static int g_interruped;

void signal_handler(int sig) { g_interruped = 1; }

int main(int argc, char* argv[]) {
  signal(SIGINT, signal_handler);

  can_filter_t filter[] = {{.iface = bus0,
                            .id = 5,
                            .mask = CAN_ERR_MASK,
                            .recv_timeout_ms = 50,
                            .send_timeout_ms = -1},
                           {.iface = bus0,
                            .id = 2,
                            .mask = CAN_ERR_MASK,
                            .recv_timeout_ms = 50,
                            .send_timeout_ms = -1}};

  can_context_params params = {
      .cb = callback,
      .filters = filter,
      .filter_cnt = sizeof(filter) / sizeof(can_filter_t)};

  snprintf(params.ifaces[0].name, sizeof(params.ifaces[0].name), "%s", bus0);

  void* ctx = can_context_create(&params);

  printf("server started\n");

  while (!g_interruped) {
    can_context_service(ctx, -1);
  }

  can_context_destroy(ctx);

  printf("server stopped\n");

  return 0;
}