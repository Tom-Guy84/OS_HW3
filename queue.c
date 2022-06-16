#include "queue.h"
#include <stdlib.h>

Queue init_queue()
{
    Queue init_q = (Queue) malloc(sizeof(*init_q));
    if(init_q == NULL) exit(1); //TODO
    init_q->process_fd = -1;
    //just something to fill arrival..
    gettimeofday(&init_q->arrival, NULL);
    init_q->next = NULL;
    //^^ That's a dummy 'node'
    return init_q;
}
void push_queue(Queue q, int p_fd, struct timeval arrival)
{
    Queue new_q = (Queue) malloc(sizeof(*new_q));
    if(new_q == NULL) exit(1); //TODO
    while(q->next != NULL) { q = q->next; }
    new_q->next = NULL;
    new_q->process_fd = p_fd;

    new_q->arrival.tv_sec = arrival.tv_sec;
    new_q->arrival.tv_usec = arrival.tv_usec;

    q->next = new_q;
}

int head_queue(Queue q)
{
    if(get_size_queue(q) == 0) return -1;
    return q->next->process_fd;
}

int pop_queue(Queue q)
{
    if(get_size_queue(q) == 0) return -1;
    int p_fd = head_queue(q);
    remove_queue(q, p_fd);
    return p_fd;
}
void remove_queue(Queue q, int p_fd)
{
    if(p_fd == -1) return;
    while(q->next != NULL && q->next->process_fd != p_fd)
    {
        q = q->next;
    }
    if(q->next == NULL) return;

    Queue toRemove = q->next;
    q->next = toRemove->next;
    free(toRemove);
}
int get_size_queue(Queue q)
{
    int count = -1;
    while(q != NULL)
    {
        q = q->next;
        count++;
    }
    return count;
}


int* get_fds_queue(Queue q)
{
    int size = get_size_queue(q);
    int* fds = (int*) malloc(sizeof(int) * size);
    for(int i = 0; i < size; i++)
    {
        fds[i] = q->next->process_fd;
        q = q->next;
    }
    return fds;
}


struct timeval find_arrival_queue(Queue q, int p_fd)
{
    while(q->next != NULL && q->next->process_fd != p_fd)
    { q = q->next; }
    //we're asserting the q isn't empty
    return q->next->arrival; 
}



void destroy_queue(Queue q)
{
    while(pop_queue(q) != -1) {}
    free(q);
}