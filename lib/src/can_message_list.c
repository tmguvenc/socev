#include "can_message_list.h"

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "can_message.h"
#include "client.h"

typedef struct {
  uint16_t max_cnt;
  uint16_t cnt;
  void** list;
} can_message_list_t;

void* can_message_list_create(uint16_t cnt) {
  can_message_list_t* ml = NULL;
  uint16_t idx;

  if (cnt == 0) {
    fprintf(stderr, "invalid maximum can message number: %u\n", cnt);
    return NULL;
  }

  ml = (can_message_list_t*)calloc(1, sizeof(can_message_list_t));
  if (!ml) {
    fprintf(stderr, "cannot create can message list object\n");
    goto create_err;
  }

  ml->max_cnt = cnt;
  ml->cnt = 0;
  ml->list = (void**)calloc(cnt, sizeof(void*));
  if (!ml->list) {
    fprintf(stderr, "cannot create can message list\n");
    goto create_err;
  }

  return ml;

create_err:
  can_message_list_destroy(ml);
  return NULL;
}

void can_message_list_destroy(void* ml) {
  uint16_t idx;
  can_message_list_t* list;

  if (ml) {
    list = (can_message_list_t*)ml;

    for (idx = 0; idx < list->cnt; idx++) {
      can_message_destroy(list->list[idx]);
    }

    free(list);
  }
}

uint16_t can_message_list_get_count(void* cl) {
  if (cl) {
    can_message_list_t* list = (can_message_list_t*)cl;
    return list->cnt;
  }

  return 0;
}

static inline uint16_t get_can_message_idx(can_message_list_t* list, int fd,
                                           result_t* out) {
  uint16_t idx = 0;
  for (; idx < list->cnt; ++idx) {
    if (can_message_parent_fd(list->list[idx]) == fd) {
      if (out) {
        out->type = FD_REGULAR;
      }
      break;
    }

    if (can_message_recv_timer_fd(list->list[idx]) == fd ||
        can_message_send_timer_fd(list->list[idx]) == fd) {
      if (out) {
        out->type = FD_TIMER;
      }
      break;
    }
  }
  return idx;
}

int can_message_list_get_message(void* ml, int fd, result_t* out) {
  if (!ml) {
    fprintf(stderr, "invalid list object!\n");
    return -1;
  }

  can_message_list_t* list = (can_message_list_t*)ml;
  uint16_t idx = get_can_message_idx(list, fd, out);
  if (idx > list->cnt) {
    fprintf(stderr, "can message fd [%d] not found!\n", fd);
    return -1;
  }
  if (out) {
    out->client = list->list[idx];
  }
  return 0;
}

static inline uint16_t find_next_empty_idx(can_message_list_t* list) {
  uint16_t idx = 0;
  for (; idx < list->max_cnt; ++idx) {
    if (list->list[idx] == NULL) {
      break;
    }
  }
  return idx;
}

int can_message_list_add_message(void* ml, void* msg) {
  if (!ml) {
    fprintf(stderr, "invalid list object!\n");
    return -1;
  }

  can_message_list_t* list = (can_message_list_t*)ml;

  if (list->cnt == list->max_cnt) {
    fprintf(stderr, "list is full, cannot add new can message!\n");
    return -1;
  }

  uint16_t idx = find_next_empty_idx(list);
  if (idx > list->max_cnt) {
    fprintf(stderr, "can message not found!\n");
    return -1;
  }

  list->list[idx] = msg;
  list->cnt++;
  return 0;
}
