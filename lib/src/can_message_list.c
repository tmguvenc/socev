#include "can_message_list.h"

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "can_message.h"
#include "client.h"

typedef struct {
  can_message_t* list;
  uint16_t cnt;
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
    fprintf(stderr, "cannot create can message list\n");
    goto create_err;
  }

  ml->cnt = cnt;
  ml->list = (can_message_t*)calloc(cnt, sizeof(can_message_t));
  if (!ml->list) {
    fprintf(stderr, "cannot create can message list\n");
    goto create_err;
  }

  for (idx = 0; idx < ml->cnt; idx++) {
    can_message_init(&ml->list[idx]);
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
      can_message_destroy(&list->list[idx]);
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
    if (list->list[idx].parent_fd == fd) {
      if (out) {
        out->type = FD_REGULAR;
      }
      break;
    }

    if (list->list[idx].recv_timer_id == fd ||
        list->list[idx].send_timer_id == fd) {
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
    out->client = &list->list[idx];
  }
  return 0;
}

int can_message_list_set_recv_timer(void* ml, uint8_t idx, int timer_fd,
                                    int parent_fd) {
  if (!ml) {
    fprintf(stderr, "invalid list object!\n");
    return -1;
  }
  can_message_list_t* list = (can_message_list_t*)ml;
  can_message_set_recv_timer(&list->list[idx], timer_fd, parent_fd);
  return 0;
}

int can_message_list_set_send_timer(void* ml, uint8_t idx, int timer_fd,
                                    int parent_fd) {
  if (!ml) {
    fprintf(stderr, "invalid list object!\n");
    return -1;
  }
  can_message_list_t* list = (can_message_list_t*)ml;
  can_message_set_send_timer(&list->list[idx], timer_fd, parent_fd);
  return 0;
}
