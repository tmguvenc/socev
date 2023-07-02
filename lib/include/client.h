#ifndef LIB_CLIENT_H_
#define LIB_CLIENT_H_

#include <stdint.h>

void* client_create(int efd, int fd, const char* ip, uint16_t port);
void client_destroy(void* client);
char* client_get_ip(void* client);
int client_get_fd(void* client);
int client_get_timerfd(void* client);
uint16_t client_get_port(void* client);
void client_set_timer(void* client, const uint64_t timeout_us);
void client_enable_timer(void* client, int en);
void client_callback_on_writable(void* client);
void client_clear_callback_on_writable(void* client);
int client_write(void* client, const void* data, unsigned int len);

#endif  // LIB_CLIENT_H_
