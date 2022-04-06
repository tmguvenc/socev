#include "client_info.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>

client_info *create_new(int client_fd, struct sockaddr_in *client_addr) {
  client_info *curr = (client_info *)malloc(sizeof(client_info));
  curr->fd = client_fd;
  curr->timer_fd = timerfd_create(CLOCK_REALTIME, 0);
  curr->port = client_addr->sin_port;
  strcpy(curr->ip, inet_ntoa(client_addr->sin_addr));
  curr->next = NULL;
  return curr;
}

client_info *client_info_create(client_info **head, int client_fd,
                                struct sockaddr_in *client_addr) {
  client_info *new_node = create_new(client_fd, client_addr);

  if (!(*head)) {
    (*head) = new_node;
  } else {
    client_info *curr = *head;
    while (1) {
      if (curr != NULL && curr->next != NULL) {
        curr = curr->next;
      } else {
        curr->next = new_node;
        break;
      }
    }
  }
  return new_node;
}

void client_info_delete(client_info **head, int fd) {
  client_info *curr = *head;

  if (curr && curr->fd == fd) {
    client_info *del = *head;
    *head = curr->next;
    free(del);
    return;
  }

  client_info *prev;
  while (curr && curr->fd != fd) {
    prev = curr;
    curr = curr->next;
  }

  if (curr == NULL) {
    printf("%d not found\n", fd);
    return;
  }

  prev->next = curr->next;
  free(curr);
}

void add_ci_to_fdlist(pollfd_list **list, client_info *ci) {
  pollfd_list *lst = *list;
  
  ci->pfd = &lst->fd_list[lst->count++];
  ci->pfd->fd = ci->fd;
  ci->pfd->events = POLLIN;

  ci->timer_pfd = &lst->fd_list[lst->count++];
  ci->timer_pfd->fd = ci->timer_fd;
  ci->timer_pfd->events = POLLIN;
}

void del_ci_from_fdlist(pollfd_list **list, client_info *ci) {
  pollfd_list *lst = *list;

  for (int i = 0; i < lst->count; i += 2) {
    if (lst->fd_list[i].fd == ci->fd &&
        lst->fd_list[i + 1].fd == ci->timer_fd) {

      
    }
  }
}

void client_info_pollfd_list(client_info **head, pollfd_list *list) {
  if (list) {
    client_info *curr = *head;
    while (curr) {
      curr->pfd = &list->fd_list[list->count++];
      curr->timer_pfd = &list->fd_list[list->count++];
      curr->pfd->fd = curr->fd;
      curr->timer_pfd->fd = curr->timer_fd;
      curr->pfd->events = POLLIN;
      curr->timer_pfd->events = POLLIN;
      curr = curr->next;
    }
  }
}