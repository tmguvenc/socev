#ifndef LIB_CAN_MESSAGE_H_
#define LIB_CAN_MESSAGE_H_

#include <linux/can.h>

#include "can_filter.h"

typedef struct {
  can_filter_t filter;
  int parent_fd;
  int recv_timer_id;
  int send_timer_id;
} can_message_t;

int can_message_init(can_message_t* msg);
void can_message_destroy(can_message_t* msg);
void can_message_set_recv_timer(can_message_t* msg, int timer_fd,
                                int parent_fd);
void can_message_set_send_timer(can_message_t* msg, int timer_fd,
                                int parent_fd);

#endif  // LIB_CAN_MESSAGE_H_
