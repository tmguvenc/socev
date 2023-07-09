#ifndef LIB_CAN_CONTEXT_H_
#define LIB_CAN_CONTEXT_H_

#include <linux/can.h>

#include "definitions.h"

typedef struct {
  canid_t id;
  canid_t mask;
  int recv_timeout_ms;
  int send_timeout_ms;
} can_filter_t;

typedef struct {
  const char* iface;
  callback_f cb;
  can_filter_t* filters;
  uint8_t filter_cnt;
} can_context_params;

void* can_context_create(const can_context_params* params);
void can_context_destroy(void* can_ctx);

int can_context_service(void* can_ctx, int timeout_ms);

#endif  // LIB_CAN_CONTEXT_H_
