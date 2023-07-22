#ifndef LIB_DEFINITIONS_H_
#define LIB_DEFINITIONS_H_

#include <stdint.h>

typedef enum { FD_REGULAR = 0, FD_TIMER, __MAX_FD_CNT } fd_type_t;

typedef enum {
  EVT_CLIENT_CONNECTED = 0,
  EVT_CLIENT_DISCONNECTED,
  EVT_CLIENT_WRITABLE,
  EVT_CLIENT_DATA_RECEIVED,
  EVT_CLIENT_TIMER_EXPIRED,
  __EVT_MAX_COUNT
} event_type;

typedef void (*callback_f)(const event_type ev, void* client, const void* in,
                           const uint32_t len);

#endif  // LIB_DEFINITIONS_H_
