#ifndef LIB_DEFINITIONS_H_
#define LIB_DEFINITIONS_H_

#include <stdint.h>

typedef enum {
  EVT_CLIENT_CONNECTED = 0,
  EVT_CLIENT_DISCONNECTED,
  EVT_CLIENT_WRITABLE,
  EVT_CLIENT_DATA_RECEIVED,
  EVT_CLIENT_TIMER_EXPIRED,
  __EVT_MAX_COUNT
} event_type;

typedef void (*callback_f)(const event_type ev, void* c_info, const void* in,
                           const uint32_t len);

#endif  // LIB_DEFINITIONS_H_
