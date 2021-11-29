#ifndef __TASKS_TYPES_H__
#define __TASKS_TYPES_H__

typedef enum task_status {
    INIT,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} task_status_t;

typedef enum task_return_value {
    TASK_COMPLETED,
    TASK_TO_BE_RESUMED
} task_return_value_t;

struct task;
typedef task_return_value_t (* task_routine_t) (struct task *, unsigned int);

typedef struct task_param {
    void *elem;
    struct task_param *next;
} task_param_t;

typedef struct task_state {
    task_param_t *input_list;
    task_param_t *output_list;
#ifdef WITH_DEPENDENCIES
    task_param_t *output_from_dependencies_list;
#endif
} task_state_t;

typedef struct task {
    unsigned int task_id;
    task_status_t status;
    task_routine_t fct;
    task_state_t tstate;
    unsigned int step;

#ifdef WITH_DEPENDENCIES
    unsigned int task_dependency_count; // Number of tasks this task depends on.
    unsigned int task_dependency_done;  // Number of solved dependencies.
    struct task *parent_task;           // Task that depends on this task.
    pthread_mutex_t task_m;
    pthread_cond_t task_cond;
#endif
} task_t;

typedef struct system_state {
    long unsigned int task_counter;
    long unsigned int task_terminated;
} system_state_t;

#endif
