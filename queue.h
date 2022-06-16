#ifndef QUEUE_H
#define QUEUE_H

#include "segel.h"

typedef struct queue
{
    int process_fd;

    struct timeval arrival;

    struct queue* next;
} *Queue;

Queue init_queue();
void push_queue(Queue q, int p_fd, struct timeval arrival);
int head_queue(Queue q);
int pop_queue(Queue q);
void remove_queue(Queue q, int p_fd);
int get_size_queue(Queue q);
int* get_fds_queue(Queue q);
struct timeval find_arrival_queue(Queue q, int p_fd);


void destroy_queue(Queue q);


#endif //QUEUE_H