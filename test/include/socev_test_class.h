#ifndef TEST_SOCEV_TEST_CLASS_H_
#define TEST_SOCEV_TEST_CLASS_H_

#include <vector>
#include <algorithm>

extern "C" {
  #include "tcp_context.h"
}

namespace test {
class socev_test_class {
public:
  explicit socev_test_class(const tcp_context_params& params);
  ~socev_test_class();

  bool service(const int32_t timeout_ms);

  socev_test_class(const socev_test_class&) = delete;
  socev_test_class(socev_test_class&&) = delete;
  socev_test_class& operator=(const socev_test_class&) = delete;
  socev_test_class& operator=(socev_test_class&&) = delete;

private:
  void* ctx_;
};
} // namespace test

#endif // TEST_SOCEV_TEST_CLASS_H_
