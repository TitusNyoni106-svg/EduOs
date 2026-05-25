#include <stdio.h>
#include <string.h>
#include "eduos.h"

void createProcess(Process *p, int pid, const char name[], int burst, int priority) {
    p->pid = pid;
    strcpy(p->name, name);

    p->state = READY;

    p->burst_time = burst;
    p->priority = priority;
}

void showProcess(Process p) {
    printf("\nPID: %d\n", p.pid);
    printf("Name: %s\n", p.name);

    if (p.state == READY)
        printf("State: READY\n");
    else if (p.state == RUNNING)
        printf("State: RUNNING\n");
    else if (p.state == TERMINATED)
        printf("State: TERMINATED\n");

    printf("Burst Time: %d\n", p.burst_time);
    printf("Priority: %d\n", p.priority);
}

void stopProcess(Process *p) {
    p->state = TERMINATED;
}