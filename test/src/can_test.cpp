#include <gtest/gtest.h>
extern "C" {
#include "can_context.h"
}

static constexpr auto bus0 = "vcan0";
static constexpr auto bus1 = "vcan1";

TEST(can_context, create_destroy) {
  can_context_params params = {.cb =
                                   [](const event_type ev, void* c_info,
                                      const void* in, const unsigned int len) {

                                   },
                               .filters = nullptr,
                               .filter_cnt = 0};

  snprintf(params.ifaces[0].name, sizeof(params.ifaces[0].name), "%s", bus0);

  auto ctx = can_context_create(&params);
  ASSERT_NE(ctx, nullptr);
  can_context_destroy(ctx);
}

TEST(can_context, filters) {
  can_filter_t filter = {.iface = bus0,
                         .id = 5,
                         .mask = CAN_ERR_MASK,
                         .recv_timeout_ms = 50,
                         .send_timeout_ms = -1};

  can_context_params params = {.cb =
                                   [](const event_type ev, void* c_info,
                                      const void* in, const unsigned int len) {

                                   },
                               .filters = &filter,
                               .filter_cnt = 1};

  snprintf(params.ifaces[0].name, sizeof(params.ifaces[0].name), "%s", bus0);

  auto ctx = can_context_create(&params);
  ASSERT_NE(ctx, nullptr);
  can_context_destroy(ctx);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
