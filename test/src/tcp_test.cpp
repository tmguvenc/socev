#include <gtest/gtest.h>
extern "C" {
#include "tcp_context.h"
}

#include <cstdint>

TEST(tcp_context, create_destroy) {
  tcp_context_params params = {
      .port = 9000,
      .max_client_count = 2,
      .callback = [](const event_type ev, void* c_info, const void* in,
                     const uint32_t len) {}};

  auto ctx = socev_create_tcp_context(params);
  ASSERT_NE(ctx, nullptr);
  socev_destroy_tcp_context(ctx);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
