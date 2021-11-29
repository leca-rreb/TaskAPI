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
__thread int tid = 0;
pthread_mutex_t dispatch_mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread_routine(void *arg)
{

    tid = * (int *) arg;

    while (1) {
        pthread_mutex_lock(&tqueue[tid]->m);

        if (tqueue[tid]->index == tqueue[tid]->begin_index) {
            while(all_queue_empty()){
                pthread_cond_wait(&tqueue[tid]->empty, &tqueue[tid]->m);
            }
            pthread_mutex_unlock(&tqueue[tid]->m);
            
            int id = idToQueue;
            pthread_mutex_lock(&tqueue[id]->m);
            while(tqueue[id]->index == tqueue[id]->begin_index){
                pthread_mutex_unlock(&tqueue[id]->m);
                pthread_mutex_lock(&dispatch_mutex);
                while(idToQueue == tid){
                    idToQueue = (idToQueue + 1) % THREAD_COUNT;
                }
                pthread_mutex_unlock(&dispatch_mutex);
                id = idToQueue;
                pthread_mutex_lock(&tqueue[id]->m);
            }
            active_task = steal_task_to_execute(id);
            
            pthread_cond_signal(&tqueue[id]->full);
            pthread_mutex_unlock(&tqueue[id]->m);
            //printf("QUEUE : %i task: %i \n", id, t->task_id);
    
        }else{
             active_task = get_task_to_execute(tid);
             pthread_cond_signal(&tqueue[tid]->full);
             pthread_mutex_unlock(&tqueue[tid]->m);
        }
       

       

        task_return_value_t ret = exec_task(active_task);
        if (ret == TASK_COMPLETED) {
            terminate_task(active_task);
            
            pthread_mutex_lock(&submit_mutex);
            sys_state.task_terminated++;
            if(sys_state.task_terminated == sys_state.task_counter){
                pthread_cond_signal(&waitall_cond);
            }
            pthread_mutex_unlock(&submit_mutex);
            //printf("Task terminated : End 2 \n");
        }
#ifdef WITH_DEPENDENCIES
        else{
            active_task->status = WAITING;
            pthread_cond_signal(&active_task->task_cond);
        }
#endif
    }

}

void create_queues(void)
{
    tqueue = (tasks_queue_t **) malloc(THREAD_COUNT * sizeof(tasks_queue_t *));
    for (int i = 0; i < THREAD_COUNT; i++)
        tqueue[i] = create_tasks_queue();

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
        threadIds[i] = i;
        pthread_create(&tids[i], NULL, thread_routine, &threadIds[i]);
    }

    return ;
}

void dispatch_task(task_t *t)
{

    //printf("%i", tid);

    pthread_mutex_lock(&tqueue[tid]->m);
    while (tqueue[tid]->index == tqueue[tid]->task_buffer_size) {
        
#ifdef WITH_DEPENDENCIES
        if (t->parent_task != NULL) {
            tqueue[tid]->task_buffer_size *= 2;
            tqueue[tid]->task_buffer = realloc(tqueue[tid]->task_buffer, sizeof(task_t *) * tqueue[tid]->task_buffer_size);
            if(tqueue[tid]->task_buffer == NULL){
                fprintf(stderr, "ERROR: the queue of tasks is full\n");
                exit(EXIT_FAILURE);
            }
        } else {
#endif
            pthread_cond_wait(&tqueue[tid]->full, &tqueue[tid]->m);
#ifdef WITH_DEPENDENCIES
        }
#endif
    }
    enqueue_task(tqueue[tid], t);

    pthread_cond_signal(&tqueue[tid]->empty);
    pthread_mutex_unlock(&tqueue[tid]->m);

}

task_t* get_task_to_execute(int id) { return dequeue_task(tqueue[id]); }

task_t* steal_task_to_execute(int id) { return dequeue_task_b(tqueue[id]); }

int all_queue_empty(){
    for(int i =0 ; i < THREAD_COUNT; i++){
        if(tqueue[i]->index != tqueue[i]->begin_index ){
            return 0;
        }
    }
    return 1;
}

int tasks_completed(void)
{
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
        pthread_mutex_lock(&waiting_task->task_m);
        while(waiting_task->status !=WAITING){
            pthread_cond_wait(&waiting_task->task_cond, &waiting_task->task_m);
        }
        waiting_task->task_dependency_done++;
        pthread_mutex_unlock(&waiting_task->task_m);
        task_check_runnable(waiting_task);
        
       
        
        
    }
#endif

}

void task_check_runnable(task_t *t)
{

#ifdef WITH_DEPENDENCIES
    if (t->task_dependency_done == t->task_dependency_count && t->status == WAITING){
        t->status = READY;
        dispatch_task(t);
    }
#endif

}
