#include "client_info.h"
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <string.h>

client_info *client_info_create(client_info **head, int client_fd,
                                struct sockaddr_in *client_addr) {
  if(!(*head)) {
    (*head) = (client_info *)malloc(sizeof(client_info));
    (*head)->fd = client_fd;
    (*head)->timer_fd = timerfd_create(CLOCK_REALTIME, 0);
    (*head)->port = client_addr->sin_port;
    strcpy((*head)->ip, inet_ntoa(client_addr->sin_addr));
    (*head)->next = NULL;
    return *head;
  }

  client_info *curr = *head;
  while(curr != NULL) {
    curr = curr->next;
  }

  curr = (client_info *)malloc(sizeof(client_info));
  curr->fd = client_fd;
  curr->timer_fd = timerfd_create(CLOCK_REALTIME, 0);
  curr->port = client_addr->sin_port;
  strcpy(curr->ip, inet_ntoa(client_addr->sin_addr));
  curr->next = NULL;

  return curr;
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

pollfd_list client_info_pollfd_list(client_info **head) {
  pollfd_list list;
  memset(&list, 0, sizeof(list));

  client_info* curr = *head;
  while(curr) {
    struct pollfd* pfd = &list.fd_list[list.count++];
    pfd->fd = curr->fd;
    printf("adding %d to fd list\n", pfd->fd);
    pfd->events = POLLIN;
    curr = curr->next;
  }

  return list;
}