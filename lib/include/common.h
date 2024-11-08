#ifndef LIB_COMMON_H_
#define LIB_COMMON_H_

#include <stdint.h>

typedef enum {
  EVT_CLIENT_CONNECTED = 0,
  EVT_CLIENT_DISCONNECTED,
  EVT_CLIENT_WRITABLE,
  EVT_CLIENT_DATA_RECEIVED,
  EVT_CLIENT_TIMER_EXPIRED,
  __EVT_MAX_COUNT
} event_type;

typedef enum {
  SOCKET_TYPE_TCP = 0,
  SOCKET_TYPE_UDP,
  __SOCKET_TYPE_MAX_COUNT,
} socket_type;

typedef void (*callback_f)(const event_type ev, void* client, const void* in,
                           const uint32_t len);

#endif  // LIB_COMMON_H_
