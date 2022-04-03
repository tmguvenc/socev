#ifndef SOCEV_LIB_TCP_LISTENER_H_
#define SOCEV_LIB_TCP_LISTENER_H_

typedef struct {

  /** @brief port number to listen */
  unsigned short port;

} tcp_listener_params;

int socev_start_listening(tcp_listener_params params);

#endif // SOCEV_LIB_TCP_LISTENER_H_
