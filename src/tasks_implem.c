#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "tasks_implem.h"
#include "tasks.h"
#include "tasks_queue.h"
#include "debug.h"

//tasks_queue_t *tqueue= NULL;
pthread_t tids[THREAD_COUNT];
tasks_queue_t **tqueue = NULL;
int idToQueue = 0;


pthread_mutex_t* m = NULL;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t* full = NULL;
pthread_cond_t* empty = NULL;

void *thread_routine(void *arg)
{
    int i= * (int *) arg;
    while (1) {
        pthread_mutex_lock(&m[i]);

        while (tqueue[i]->index == 0) { pthread_cond_wait(&empty[i], &m[i]); }

        active_task = get_task_to_execute(i);
        pthread_cond_signal(&full[i]);
        pthread_mutex_unlock(&m[i]);
        pthread_mutex_lock(&m2);

        task_return_value_t ret = exec_task(active_task);
        if (ret == TASK_COMPLETED) {
            terminate_task(active_task);
            sys_state.task_terminated++;
        }
#ifdef WITH_DEPENDENCIES
        else
            active_task->status = WAITING;
#endif
       pthread_mutex_unlock(&m2);
    }
}

void create_queues(void)
{
    m = (pthread_mutex_t *) malloc(THREAD_COUNT * sizeof(pthread_mutex_t));
    full = (pthread_cond_t *) malloc(THREAD_COUNT * sizeof(pthread_cond_t));
    empty = (pthread_cond_t *) malloc(THREAD_COUNT * sizeof(pthread_cond_t));

    tqueue = (tasks_queue_t**) malloc(THREAD_COUNT * sizeof(tasks_queue_t*));
    for( int i =0; i < THREAD_COUNT; i++){
        tqueue[i] = create_tasks_queue();
    }
}

void delete_queues(void)
{
    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        free_tasks_queue(tqueue[i]);
    }

}

void create_thread_pool(void)
{
    int *threadIds = (int*) malloc(THREAD_COUNT* sizeof(int));
    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        m[i] = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
        full[i] = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
        empty[i] = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
        threadIds[i] = i;
        pthread_create(&tids[i], NULL, thread_routine, &threadIds[i]);
    }

    return ;
}

void dispatch_task(task_t *t)
{

    pthread_mutex_lock(&m[idToQueue]);
    int id = idToQueue;
    idToQueue = (idToQueue +1)%THREAD_COUNT;
    while (tqueue[id]->index == tqueue[id]->task_buffer_size)
    {
    #ifdef WITH_DEPENDENCIES
    if(t->parent_task != NULL){
        tqueue[id]->task_buffer_size = tqueue[id]->task_buffer_size*2;
        tqueue[id]->task_buffer = realloc(tqueue[id]->task_buffer, sizeof(task_t*) * tqueue[id]->task_buffer_size);
        if(tqueue[id]->task_buffer == NULL){
            fprintf(stderr, "ERROR: the queue of tasks is full\n");
            exit(EXIT_FAILURE);
        }
    }else{
    #endif
        pthread_cond_wait(&full[id], &m[id]);
    #ifdef WITH_DEPENDENCIES
    }
    #endif

    }
    enqueue_task(tqueue[id], t);

    pthread_cond_signal(&empty[id]);
    pthread_mutex_unlock(&m[id]);

}

task_t* get_task_to_execute(int id) { return dequeue_task(tqueue[id]); }

int tasks_completed(void)
{

    // printf("%ld", sys_state.task_counter);
    if (sys_state.task_terminated == sys_state.task_counter)
        return 1;
    return 0;

}

unsigned int exec_task(task_t *t)
{

    t->step++;
    t->status = RUNNING;

    PRINT_DEBUG(10, "Execution of task %u (step %u)\n", t->task_id, t->step);
    unsigned int result = t->fct(t, t->step);

    return result;

}

void terminate_task(task_t *t)
{

    t->status = TERMINATED;

    PRINT_DEBUG(10, "Task terminated: %u\n", t->task_id);

#ifdef WITH_DEPENDENCIES
    if (t->parent_task != NULL) {
        task_t *waiting_task = t->parent_task;
        waiting_task->task_dependency_done++;
        task_check_runnable(waiting_task);
    }
#endif

}

void task_check_runnable(task_t *t)
{

#ifdef WITH_DEPENDENCIES
    if (t->task_dependency_done == t->task_dependency_count){
        t->status = READY;
        dispatch_task(t);
    }
#endif

}
