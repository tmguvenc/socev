#ifndef LIB_CAN_MESSAGE_H_
#define LIB_CAN_MESSAGE_H_

#include "can_filter.h"

typedef struct {
  can_filter_t filter;
} can_message_t;

void* can_context_create(const can_context_params* params);
void can_context_destroy(void* can_ctx);
int can_context_service(void* can_ctx, int timeout_ms);

#endif  // LIB_CAN_MESSAGE_H_
