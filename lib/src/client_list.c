#include "client_list.h"

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "client.h"

typedef struct {
  uint16_t max_cnt;
  uint16_t cnt;
  void** list;
} client_list_t;

void* client_list_create(uint16_t cnt) {
  client_list_t* cl = NULL;

  if (cnt == 0) {
    fprintf(stderr, "invalid maximum client number: %u\n", cnt);
    return NULL;
  }

  cl = (client_list_t*)calloc(1, sizeof(client_list_t));
  if (!cl) {
    fprintf(stderr, "cannot create client list\n");
    goto create_err;
  }

  cl->max_cnt = cnt;
  cl->cnt = 0;
  cl->list = (void**)calloc(cnt, sizeof(void*));
  if (!cl->list) {
    fprintf(stderr, "cannot create client list\n");
    goto create_err;
  }

  return cl;

create_err:
  client_list_destroy(cl);
  return NULL;
}

void client_list_destroy(void* cl) {
  if (cl) {
    client_list_t* list = (client_list_t*)cl;
    uint16_t i = 0;

    for (; i < list->max_cnt; i++) {
      client_destroy(list->list[i]);
    }

    free(list);
  }
}

uint16_t client_list_get_count(void* cl) {
  if (cl) {
    client_list_t* list = (client_list_t*)cl;
    return list->cnt;
  }

  return 0;
}

uint16_t client_list_get_max_count(void* cl) {
  if (cl) {
    client_list_t* list = (client_list_t*)cl;
    return list->max_cnt;
  }

  return 0;
}

bool client_list_is_full(void* cl) {
  if (cl) {
    client_list_t* list = (client_list_t*)cl;
    return list->cnt == list->max_cnt;
  }

  return false;
}

bool client_list_is_empty(void* cl) {
  if (cl) {
    client_list_t* list = (client_list_t*)cl;
    return list->cnt == 0;
  }

  return false;
}

static inline uint16_t find_next_empty_idx(client_list_t* list) {
  uint16_t idx = 0;
  for (; idx < list->max_cnt; ++idx) {
    if (list->list[idx] == NULL) {
      break;
    }
  }
  return idx;
}

bool client_list_add_client(void* cl, void* ci) {
  if (!cl) {
    fprintf(stderr, "invalid list object!\n");
    return false;
  }

  client_list_t* list = (client_list_t*)cl;

  if (list->cnt == list->max_cnt) {
    fprintf(stderr, "list is full, cannot add new client!\n");
    return false;
  }

  uint16_t idx = find_next_empty_idx(list);
  if (idx > list->max_cnt) {
    fprintf(stderr, "client not found!\n");
    return false;
  }

  list->list[idx] = ci;
  list->cnt++;
  return true;
}

static inline uint16_t get_client_idx(client_list_t* list, int fd,
                                      client_get_result_t* out) {
  uint16_t idx = 0;
  for (; idx < list->max_cnt; ++idx) {
    if (client_get_fd(list->list[idx]) == fd) {
      if (out) {
        out->type = FD_REGULAR;
      }
      break;
    }

    if (client_get_timerfd(list->list[idx]) == fd) {
      if (out) {
        out->type = FD_TIMER;
      }
      break;
    }
  }
  return idx;
}

bool client_list_del_client(void* cl, int client_fd) {
  if (!cl) {
    fprintf(stderr, "invalid list object!\n");
    return false;
  }

  client_list_t* list = (client_list_t*)cl;

  if (list->cnt == 0) {
    fprintf(stderr, "list is empty, cannot delete the client!");
    return false;
  }

  uint16_t idx = get_client_idx(list, client_fd, NULL);
  if (idx > list->max_cnt) {
    fprintf(stderr, "client fd [%d] not found!\n", client_fd);
    return false;
  }

  client_destroy(list->list[idx]);
  list->list[idx] = NULL;
  list->cnt--;
  return true;
}

int client_list_get_client(void* cl, int fd, client_get_result_t* out) {
  if (!cl) {
    fprintf(stderr, "invalid list object!\n");
    return -1;
  }

  client_list_t* list = (client_list_t*)cl;
  uint16_t idx = get_client_idx(list, fd, out);
  if (idx > list->max_cnt) {
    fprintf(stderr, "client fd [%d] not found!\n", fd);
    return -1;
  }
  if (out) {
    out->client = list->list[idx];
  }
  return 0;
}
