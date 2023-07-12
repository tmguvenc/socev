#ifndef LIB_CAN_MESSAGE_H_
#define LIB_CAN_MESSAGE_H_

#include <linux/can.h>

void* can_message_create(const struct can_filter* filter, int recv_timer_id,
                         int send_timer_id);
void can_message_destroy(void* msg);

#endif  // LIB_CAN_MESSAGE_H_
