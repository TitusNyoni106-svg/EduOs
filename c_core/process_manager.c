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
void edu_ps(Process p[])
{
    printf("\n=== EDUOS PROCESS LIST ===\n");

    showProcess(p[0]);
    showProcess(p[1]);
    showProcess(p[2]);
}
// ================= SYSTEM CALLS =================

void edu_fork(Process *parent, Process *child, int new_pid)
{
    child->pid = new_pid;
    strcpy(child->name, parent->name);
    child->state = READY;
    child->burst_time = parent->burst_time;
    child->priority = parent->priority;

    printf("\n[FORK] Process %d created from Process %d\n",
           child->pid, parent->pid);
}

void edu_exec(Process *p, const char new_name[])
{
    strcpy(p->name, new_name);

    printf("\n[EXEC] Process %d now running %s\n",
           p->pid, p->name);
}

void edu_wait(Process *p)
{
    printf("\n[WAIT] Process %d is waiting...\n", p->pid);
    p->state = READY;
}

void edu_exit(Process *p)
{
    printf("\n[EXIT] Process %d terminated\n", p->pid);
    p->state = TERMINATED;
}