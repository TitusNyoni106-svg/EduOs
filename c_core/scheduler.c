#include <stdio.h>
#include "eduos.h"

void fcfs(Process p[], int n) {

    printf("\n=== FCFS Scheduling ===\n");

    for (int i = 0; i < n; i++) {

        // READY → RUNNING
        p[i].state = RUNNING;

        printf("\nProcess %d (%s) is RUNNING\n", p[i].pid, p[i].name);

        for (int j = 0; j < p[i].burst_time; j++) {
            printf(".");
        }

        // RUNNING → TERMINATED
        p[i].state = TERMINATED;

        printf(" DONE → TERMINATED\n");
    }

    printf("\nAll processes completed.\n");
}