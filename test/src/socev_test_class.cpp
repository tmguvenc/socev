#include "socev_test_class.h"
#include "tcp_context.h"

namespace test {

socev_test_class::socev_test_class(const tcp_context_params &params)
    : ctx_(socev_create_tcp_context(params)) {
  cb_map_.resize(5, nullptr);
}

socev_test_class::~socev_test_class() { socev_destroy_tcp_context(ctx_); }

bool socev_test_class::service(const int32_t timeout_ms) {
  return socev_service(ctx_, timeout_ms) != -1;
}

} // namespace test
