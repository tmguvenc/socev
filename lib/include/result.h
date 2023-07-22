#ifndef LIB_RESULT_H_
#define LIB_RESULT_H_

#include "definitions.h"

typedef struct {
  fd_type_t type;
  void* client;
} result_t;

#endif  // LIB_RESULT_H_
