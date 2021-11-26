#include <stdlib.h>

#include <pthread.h>

#include "utils.h"
#include "debug.h"
#include "tasks.h"
#include "tasks_implem.h"

system_state_t sys_state;
__thread task_t *active_task;
pthread_mutex_t submit_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitall_cond = PTHREAD_COND_INITIALIZER;

void runtime_init(void)
{

    // A random number generator might be useful towards the end of the lab.
    rand_generator_init();

    create_queues();
    create_thread_pool();

    sys_state.task_counter = 0;
    sys_state.task_terminated = 0;

}

void runtime_init_with_deps(void)
{

#ifndef WITH_DEPENDENCIES
    fprintf(stderr, "ERROR: dependencies are not supported by the runtime. "
                    "This application cannot be executed\n");
    exit(EXIT_FAILURE);
#endif
    runtime_init();

}

void runtime_finalize(void)
{

    task_waitall();
    PRINT_DEBUG(1, "Terminating ... \t Total task count: %lu \n",
        sys_state.task_counter);
    delete_queues();

}

task_t *create_task(task_routine_t f)
{

    task_t *t = malloc(sizeof(task_t));
     pthread_mutex_lock(&submit_mutex);
    t->task_id = ++sys_state.task_counter;
    pthread_mutex_unlock(&submit_mutex);
    t->fct = f;
    t->step = 0;

    t->tstate.input_list = NULL;
    t->tstate.output_list = NULL;

#ifdef WITH_DEPENDENCIES
    t->tstate.output_from_dependencies_list = NULL;
    t->task_dependency_count = 0;
    t->task_dependency_done = 0;
    t->parent_task = NULL;
#endif

    t->status = INIT;

    PRINT_DEBUG(10, "task created with id %u\n", t->task_id);

    return t;

}

void submit_task(task_t *t)
{

    t->status = READY;
#ifdef WITH_DEPENDENCIES
    if (active_task != NULL) {
        t->parent_task = active_task;
        pthread_mutex_lock(&submit_mutex);
        active_task->task_dependency_count++;
        pthread_mutex_unlock(&submit_mutex);
        PRINT_DEBUG(100, "Dependency %u -> %u\n",
            active_task->task_id, t->task_id);
    }
#endif


    dispatch_task(t);

}

void task_waitall(void) { while(sys_state.task_terminated != sys_state.task_counter) {
    pthread_cond_wait(&waitall_cond, &submit_mutex);
} }
