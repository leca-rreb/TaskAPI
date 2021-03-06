#include <stdio.h>
#include <stdlib.h>

#include "tasks_queue.h"

tasks_queue_t *create_tasks_queue(void)
{

    tasks_queue_t *q = (tasks_queue_t *) malloc(sizeof(tasks_queue_t));

    q->task_buffer_size = QUEUE_SIZE;
    q->task_buffer = (task_t **) malloc(q->task_buffer_size * sizeof(task_t *));
    q->index = 0;
    q->begin_index = 0;
    q->m = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    q->full = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    q->empty = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    return q;

}

void free_tasks_queue(tasks_queue_t *q)
{

    /*
        IMPORTANT: We chose not to free the queues to simplify the
        termination of the program (and make debugging less complex).
    */

    // free(q->task_buffer);
    // free(q);

}

void enqueue_task(tasks_queue_t *q, task_t *t)
{
    q->task_buffer[q->index] = t;
    q->index++;


   /*  if(q->index == q->task_buffer_size){
        fprintf(stderr, "ERROR: the queue of tasks is full\n");
        exit(EXIT_FAILURE);
    }
    */
    return;

}

task_t *dequeue_task(tasks_queue_t *q)
{

    if (q->index == q->begin_index)
        return NULL;

    task_t *t = q->task_buffer[q->index-1];
    q->index--;

    return t;

}

task_t *dequeue_task_b(tasks_queue_t *q)
{

    if (q->index == q->begin_index)
        return NULL;

    task_t *t = q->task_buffer[q->begin_index];
    q->begin_index++;

    return t;

}
