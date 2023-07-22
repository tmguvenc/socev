#ifndef LIB_CAN_MESSAGE_H_
#define LIB_CAN_MESSAGE_H_

#include <linux/can.h>
#include <stdint.h>

void* can_message_create(int efd, int parent_fd, int recv_timeout_ms,
                         int send_timeout_ms);
void can_message_destroy(void* msg);
int can_message_start_recv_timer(const void* msg);
int can_message_stop_recv_timer(const void* msg);
int can_message_start_send_timer(const void* msg);
int can_message_stop_send_timer(const void* msg);
int can_message_recv_timeout(const void* msg);
int can_message_send_timeout(const void* msg);
int can_message_parent_fd(const void* msg);
int can_message_recv_timer_fd(const void* msg);
int can_message_send_timer_fd(const void* msg);
uint8_t can_message_active_fd_cnt(const void* msg);

#endif  // LIB_CAN_MESSAGE_H_
