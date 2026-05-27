#include <stdio.h>
#include "eduos.h"

// ================= FCFS =================
void fcfs(Process p[], int n)
{
    printf("\n=== FCFS Scheduling ===\n");

    for (int i = 0; i < n; i++) {

        // Skip terminated processes
        if (p[i].state == TERMINATED)
            continue;

        p[i].state = RUNNING;

        printf("\nProcess %d (%s) is RUNNING\n",
               p[i].pid,
               p[i].name);

        for (int j = 0; j < p[i].burst_time; j++) {
            printf(".");
        }

        p[i].state = TERMINATED;

        printf(" DONE -> TERMINATED\n");
    }

    printf("\nFCFS Completed.\n");
}


// ================= SJF =================
void sjf(Process p[], int n)
{
    printf("\n=== SJF SCHEDULING ===\n");

    Process temp[10];

    // Copy processes
    for (int i = 0; i < n; i++) {
        temp[i] = p[i];
    }

    // Sort by burst time
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {

            if (temp[i].burst_time > temp[j].burst_time) {

                Process t = temp[i];
                temp[i] = temp[j];
                temp[j] = t;
            }
        }
    }

    // Execute
    for (int i = 0; i < n; i++) {

        // Skip terminated processes
        if (temp[i].state == TERMINATED)
            continue;

        temp[i].state = RUNNING;

        printf("\nProcess %d (%s) is RUNNING\n",
               temp[i].pid,
               temp[i].name);

        for (int k = 0; k < temp[i].burst_time; k++) {
            printf(".");
        }

        temp[i].state = TERMINATED;

        printf(" DONE -> TERMINATED\n");
    }

    printf("\nSJF Completed.\n");
}


// ================= PRIORITY =================
void priority_scheduling(Process p[], int n)
{
    printf("\n=== PRIORITY SCHEDULING ===\n");

    Process temp[10];

    // Copy processes
    for (int i = 0; i < n; i++) {
        temp[i] = p[i];
    }

    // Sort by priority
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {

            if (temp[i].priority > temp[j].priority) {

                Process t = temp[i];
                temp[i] = temp[j];
                temp[j] = t;
            }
        }
    }

    // Execute
    for (int i = 0; i < n; i++) {

        // Skip terminated processes
        if (temp[i].state == TERMINATED)
            continue;

        temp[i].state = RUNNING;

        printf("\nProcess %d (%s) is RUNNING\n",
               temp[i].pid,
               temp[i].name);

        for (int k = 0; k < temp[i].burst_time; k++) {
            printf(".");
        }

        temp[i].state = TERMINATED;

        printf(" DONE -> TERMINATED\n");
    }

    printf("\nPriority Scheduling Completed.\n");
}