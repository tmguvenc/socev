#include "stdio.h"
#include "tcp_context.h"
#include <string.h>

int main(int argc, char* argv[]) {

  tcp_context_params params;
  memset(&params, 0, sizeof(params));

  params.port = 9000;
  params.max_client_count = 10;

  void* ctx = socev_create_tcp_context(params);

  printf("server started\n");

  socev_destroy_tcp_context(ctx);

  printf("server stopped\n");

  return 0;
}