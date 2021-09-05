#pragma once

typedef enum {
    start,
    stop,
    run,
    sleep,
    wake,
    newProcess
} taskType;

typedef enum {
    running,
    sleeping,
    stopped
} processStatus;

struct processStruct;

typedef struct {
    struct processStruct *depProcess;
    processStatus depStatus;    
} dependency;

typedef struct {
    void (*task)();
    taskType type;
    int depCnt;
    dependency *dependencies; // ignored on type run
} task;

typedef struct processStruct {
    int taskCnt;
    task *tasks;
    volatile _Bool *trigger;
    processStatus status; // Don't touch this, used by system functions
} process;

void system_task (taskType type, process *Process);
void system_run ();
