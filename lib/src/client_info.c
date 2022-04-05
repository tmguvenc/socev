#include "client_info.h"
#include <stdlib.h>
#include <stdio.h>

client_info_list* client_info_list_create(unsigned int count) {
  client_info_list* list = (client_info_list*)malloc(count * sizeof(client_info_list));
  if(!list) {
    fprintf(stderr, "cannot create client info list\n");
    return NULL;
  }

  return list;
}

void client_info_list_destroy(client_info_list* list) {
  if(list) {
    free(list);
  }
}
