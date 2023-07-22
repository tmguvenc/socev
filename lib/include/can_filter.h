#ifndef LIB_CAN_FILTER_H_
#define LIB_CAN_FILTER_H_

#include <linux/can.h>

/** @brief A struct for can filters */
typedef struct {
  /** @brief bus name */
  const char* iface;

  /** @brief can id of the filter */
  canid_t id;

  /** @brief can filter mask */
  canid_t mask;

  /** @brief message receive timeout in milliseconds */
  int recv_timeout_ms;

  /** @brief message send timeout in milliseconds */
  int send_timeout_ms;
} can_filter_t;

#endif  // LIB_CAN_FILTER_H_
