#include <stdio.h>
#include <pthread.h>

#include "tasks_implem.h"
#include "tasks.h"
#include "tasks_queue.h"
#include "debug.h"

tasks_queue_t *tqueue= NULL;
pthread_t tids[THREAD_COUNT];

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t  empty = PTHREAD_COND_INITIALIZER;

void *thread_routine(void *arg)
{

    while (1) {
        pthread_mutex_lock(&m);

        while (tqueue->index == 0)
        {
            pthread_cond_wait(&empty, &m);
        }

        active_task = get_task_to_execute();
        task_return_value_t ret = exec_task(active_task);
        if (ret == TASK_COMPLETED) {
            terminate_task(active_task);
            sys_state.task_terminated++;
        }
        pthread_cond_signal(&full);
        pthread_mutex_unlock(&m);
    }
    
}

void create_queues(void)
{
    tqueue = create_tasks_queue();
}

void delete_queues(void)
{
    free_tasks_queue(tqueue);
}    

void create_thread_pool(void)
{
    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&tids[i], NULL, thread_routine, NULL);
    }
    
    return ;
}

void dispatch_task(task_t *t)
{
    enqueue_task(tqueue, t);
}

task_t* get_task_to_execute(void)
{
    return dequeue_task(tqueue);
}

int tasks_completed(void)
{
    //printf("%ld", sys_state.task_counter);
    if(sys_state.task_terminated == sys_state.task_counter){
        return 1;
    }
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
    if(t->parent_task != NULL){
        task_t *waiting_task = t->parent_task;
        waiting_task->task_dependency_done++;
        
        task_check_runnable(waiting_task);
    }
#endif

}

void task_check_runnable(task_t *t)
{
#ifdef WITH_DEPENDENCIES
    if(t->task_dependency_done == t->task_dependency_count){
        t->status = READY;
        dispatch_task(t);
    }
#endif
}
