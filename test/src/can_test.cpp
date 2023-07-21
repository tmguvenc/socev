#include <gtest/gtest.h>
extern "C" {
#include "can_context.h"
}
TEST(can_context, create_destroy) {
  can_context_params params = {.ifaces = {"vcan0"},
                               .cb =
                                   [](const event_type ev, void* c_info,
                                      const void* in, const unsigned int len) {

                                   },
                               .filters = nullptr,
                               .filter_cnt = 0};

  auto ctx = can_context_create(&params);
  ASSERT_NE(ctx, nullptr);
  can_context_destroy(ctx);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
