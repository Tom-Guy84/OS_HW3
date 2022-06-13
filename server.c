#include "segel.h"
#include "request.h"
#include "queue.h"
// 
// server.c: A very, very simple web server (not anymore)
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

#define NOT_DISPATCHED -1

Queue waiting_q = NULL;
Queue working_q = NULL;

pthread_cond_t c;
pthread_cond_t limit_c;
pthread_mutex_t m;

typedef struct 
{
    pthread_t id;
    int req;
    int req_s;
    int req_d;
} *Thread_Stats;

Thread_Stats actual_threads;
int threads;

void* thread_function(void* arg);

// HW3: Parse the new arguments too
void getargs(int *port, int* threads, int* queues_size, char** flag, int argc, char *argv[])
{
    if (argc < 4) {
	fprintf(stderr, "Usage: %s <port> <threads> <queues_size> <schedalg>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *queues_size = atoi(argv[3]);
    *flag = (char*) malloc(strlen(argv[4] + 1));
    strcpy(*flag, argv[4]);
    //TODO free the flags when freeing the threads
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    
    int queues_size;
    char* flag;

    pthread_cond_init(&c, NULL);
    pthread_cond_init(&limit_c, NULL);
    pthread_mutex_init(&m, NULL);

    waiting_q = init_queue();
    working_q = init_queue();

    getargs(&port, &threads, &queues_size, &flag, argc, argv);
    actual_threads = (Thread_Stats) malloc(sizeof(*actual_threads) * threads);
    if(actual_threads == NULL)
    {
        exit(1); //TODO: malloc didn't work, what should we return
    }


    for(int i = 0; i < threads; i++)
    {
        pthread_create(&actual_threads[i].id, NULL, thread_function, NULL);
        actual_threads->req = actual_threads->req_d = actual_threads->req_s = 0;
    }
   
    listenfd = Open_listenfd(port);
    struct timeval t;

    srand(time(NULL)); // init random seed

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        gettimeofday(&t, NULL);
        //TODO ERROR OF TIME

        double arrival = (double) ((double)t.tv_sec + (double)t.tv_usec / 1e6); //milliseconds

        // 
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads 
        // do the work. 
        // 
        pthread_mutex_lock(&m);
        if(get_size_queue(working_q) + get_size_queue(waiting_q) >= queues_size)
        {
            if(strcmp(flag, "block") == 0)
            {
                while(get_size_queue(working_q) + get_size_queue(waiting_q) >= queues_size)
                {
                    pthread_cond_wait(&limit_c, &m);
                }
            }
            else if(strcmp(flag, "dt") == 0)
            {
                Close(connfd);
                pthread_mutex_unlock(&m);
                continue; //don't want to push the new fd
            }
            else if(strcmp(flag, "random") == 0)
            {
                int waiting_size = get_size_queue(waiting_q);
                int drop = waiting_size * 0.3 + ((waiting_size % 10 == 0) ? 0 : 1); //ceil
                
                if (drop == 0) // don't have anyone to drop, hence need to drop the new request
                {
                    Close(connfd);
                    pthread_mutex_unlock(&m);
                    continue;
                }
                if (drop == waiting_size) // have to drop the whole waiting queue
                {
					int popped_fd = 0;
                    while((popped_fd = pop_queue(waiting_q)) != -1)
                    {
						Close(popped_fd);
				    }
                }
                else
                {
                    int* fds = get_fds_queue(waiting_q);
                    for(int i = 0; i < drop; i++)
                    {
                        int rnd_value = rand() % waiting_size; // waiting_size because array's size doesn't change..
                        if(fds[rnd_value] == -1) {i--; continue;} // we already deleted that one
                        
                        remove_queue(waiting_q, fds[rnd_value]);
                        Close(fds[rnd_value]);
                        printf("popped: %d\n", rnd_value);
                        fds[rnd_value] = -1;
                    }
                    free(fds);
                }
                
            }
            else if(strcmp(flag, "dh") == 0)
            {
                int p_fd = pop_queue(waiting_q);
                if(p_fd == -1)
                {
                    Close(connfd);
                    pthread_mutex_unlock(&m);
                    continue; //Todo, if no one is in waiting queue - drop the new fd?
                }
                Close(p_fd);
            }
        }     
        push_queue(waiting_q, connfd, arrival, NOT_DISPATCHED);
        pthread_cond_signal(&c); 

        pthread_mutex_unlock(&m);
    }

}


void* thread_function(void* arg)
{
    while(1)
    {
        pthread_mutex_lock(&m);
        while(get_size_queue(waiting_q) == 0)
        {
            pthread_cond_wait(&c, &m);
        }
        struct timeval t;
        gettimeofday(&t, NULL);

        double start_disp = (double) ((double)t.tv_sec  + (double)t.tv_usec / 1e6); //milliseconds

        int p_fd = head_queue(waiting_q);
        double arrival = find_arrival_queue(waiting_q, p_fd); 

        pop_queue(waiting_q);
        push_queue(working_q, p_fd, arrival, start_disp);

        int i = 0;
        pthread_t c_id = pthread_self();
        for(; i < threads; i++)
        {
            if(actual_threads[i].id == c_id)
                break;
        }

        double temp_arrival = find_arrival_queue(working_q, p_fd);

        pthread_mutex_unlock(&m);

        int res = requestHandle(p_fd, temp_arrival, start_disp-temp_arrival, i, actual_threads[i].req, 
                                actual_threads[i].req_s, actual_threads[i].req_d);

        actual_threads[i].req++;
        if(res != ERROR_REQ)
        {
            if(res == STATIC_REQ)
            {
                actual_threads[i].req_s++;
            }
            else
            {
                actual_threads[i].req_d++;
            }
        }

        pthread_mutex_lock(&m);
        remove_queue(working_q, p_fd);
        pthread_cond_signal(&limit_c);
        pthread_mutex_unlock(&m);
        Close(p_fd);    
    }
}
