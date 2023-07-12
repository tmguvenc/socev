#ifndef LIB_CAN_CONTEXT_H_
#define LIB_CAN_CONTEXT_H_

#include <linux/can.h>

#include "can_filter.h"
#include "definitions.h"

/** @brief A struct for can context parameters */
typedef struct {
  /** @brief can interface name */
  const char* iface;

  /** @brief callback function */
  callback_f cb;

  /** @brief list of can filters */
  can_filter_t* filters;

  /** @brief can filter count */
  uint8_t filter_cnt;
} can_context_params;

void* can_context_create(const can_context_params* params);
void can_context_destroy(void* can_ctx);
int can_context_service(void* can_ctx, int timeout_ms);

#endif  // LIB_CAN_CONTEXT_H_
