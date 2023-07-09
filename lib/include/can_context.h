#ifndef LIB_CAN_CONTEXT_H_
#define LIB_CAN_CONTEXT_H_

#include "definitions.h"

typedef struct {
  const char* iface;
  callback_f cb;
} can_context_params;

void* can_context_create(can_context_params params);
void can_context_destroy(void* can_ctx);

int can_context_service(void* can_ctx, int timeout_ms);

#endif  // LIB_CAN_CONTEXT_H_
