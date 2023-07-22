#ifndef LIB_CAN_MESSAGE_LIST_H_
#define LIB_CAN_MESSAGE_LIST_H_

#include "result.h"

void* can_message_list_create(uint16_t cnt);
void can_message_list_destroy(void* ml);
uint16_t can_message_list_get_count(void* ml);
int can_message_list_get_message(void* ml, int fd, result_t* out);
int can_message_list_add_message(void* ml, void* msg);

#endif  // LIB_CAN_MESSAGE_LIST_H_
