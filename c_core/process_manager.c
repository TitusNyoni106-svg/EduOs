#include "include/eduos.h"

// ================= PROCESS FUNCTIONS =================

void createProcess(Process *p, int pid, const char name[], int burst, int priority) {
    p->pid = pid;
    strcpy(p->name, name);
    strcpy(p->state, "READY");
    p->burst_time = burst;
    p->priority = priority;
}

void showProcess(Process p) {
    printf("\nPID: %d\nName: %s\nState: %s\nBurst: %d\nPriority: %d\n",
           p.pid, p.name, p.state, p.burst_time, p.priority);
}

// ================= FCFS =================
void fcfs(Process p[], int n) {
    for (int i = 0; i < n; i++) {
        printf("\n[FCFS] Process %d (%s) RUNNING...\n", p[i].pid, p[i].name);
        printf("..... DONE -> TERMINATED\n");
        strcpy(p[i].state, "TERMINATED");
    }
}

// ================= SJF =================
void sjf(Process p[], int n) {
    Process temp;

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (p[i].burst_time > p[j].burst_time) {
                temp = p[i];
                p[i] = p[j];
                p[j] = temp;
            }
        }
    }

    fcfs(p, n);
}

// ================= PRIORITY =================
void priority_scheduling(Process p[], int n) {
    Process temp;

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (p[i].priority > p[j].priority) {
                temp = p[i];
                p[i] = p[j];
                p[j] = temp;
            }
        }
    }

    fcfs(p, n);
}