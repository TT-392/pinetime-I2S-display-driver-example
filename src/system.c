#include <stdbool.h>
#include <string.h>
#include "system.h"

static process **knownProcesses;
static int knownProcessCount = 0;

static void add_to_known_processes (process *Process) {
    for (int i = 0; i < knownProcessCount; i++) {
        if (Process == knownProcesses[i]) {
            return;
        }
    }

    knownProcessCount++;

    process **oldProcesses = knownProcesses;

    knownProcesses = (process**)malloc((knownProcessCount) * sizeof(struct process*));
    memcpy(knownProcesses, oldProcesses, (knownProcessCount - 1) * sizeof(struct process*));
    free(oldProcesses);

    knownProcesses[knownProcessCount - 1] = Process;
}

static processStatus getStatus (process *Process) {
    for (int i = 0; i < knownProcessCount; i++) {
        if (Process == knownProcesses[i]) {
            return Process->status;
        }
    }

    Process->status = stopped;
    add_to_known_processes(Process);

    return stopped;
}

void set_status (processStatus desiredStatus, process *Process) {
    processStatus currentStatus = getStatus(Process);

    while (currentStatus != desiredStatus) {
        taskType task;
        if (currentStatus == stopped)
            task = start;
        else if (currentStatus == running && desiredStatus == sleeping)
            task = sleep;
        else if (currentStatus == running && desiredStatus == stopped)
            task = stop;
        else if (currentStatus == sleeping)
            task = wake;

        system_task(task, Process);

        currentStatus = getStatus(Process);
    }
}

static void run_task (task Task, process *Process) {
    if (Task.type != run) {
        for (int i = 0; i < Task.depCnt; i++) {
            processStatus status = getStatus(Task.dependencies[i].depProcess);

            if (Task.dependencies[i].depStatus != status) {
                set_status(Task.dependencies[i].depStatus, Task.dependencies[i].depProcess);
            }
        }
    }

    Task.task();

    if (Task.type == start) Process->status = running;
    else if (Task.type == stop) Process->status = stopped;
    else if (Task.type == wake) Process->status = running;
    else if (Task.type == sleep) Process->status = sleeping;

    add_to_known_processes(Process);
}

void system_task (taskType type, process *Process) {
    for (int i = 0; i < Process->taskCnt; i++) {
        if (Process->tasks[i].type == type) {
            run_task(Process->tasks[i], Process);
        }
    }
}

bool process_has_task (taskType type, process *Process) {
    for (int i = 0; i < Process->taskCnt; i++) {
        if (Process->tasks[i].type == type) {
            return 1;
        }
    }
    return 0;
}

void system_run () {
    for (int i = 0; i < knownProcessCount; i++) {
        if (knownProcesses[i]->status == running && process_has_task(run, knownProcesses[i])) {
            system_task(run, knownProcesses[i]);
        }
    }
}
