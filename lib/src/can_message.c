#include "can_message.h"

#include <stdlib.h>
#include <unistd.h>

int can_message_init(can_message_t* msg) {
  msg->filter.iface = NULL;
  msg->filter.id = 0;
  msg->filter.mask = 0;
  msg->filter.recv_timeout_ms = -1;
  msg->filter.recv_timeout_ms = -1;
  msg->parent_fd = -1;
  msg->recv_timer_id = -1;
  msg->send_timer_id = -1;
}

void can_message_destroy(can_message_t* msg) {
  if (msg->recv_timer_id != -1) {
    close(msg->recv_timer_id);
  }
  if (msg->send_timer_id != -1) {
    close(msg->send_timer_id);
  }
}

void can_message_set_recv_timer(can_message_t* msg, int timer_fd,
                                int parent_fd) {
  msg->recv_timer_id = timer_fd;
  msg->parent_fd = parent_fd;
}

void can_message_set_send_timer(can_message_t* msg, int timer_fd,
                                int parent_fd) {
  msg->send_timer_id = timer_fd;
  msg->parent_fd = parent_fd;
}
