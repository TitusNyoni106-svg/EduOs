#include "include/eduos.h"

int main() {

    Process p[3];

    createProcess(&p[0], 1, "System Idle", 5, 1);
    createProcess(&p[1], 2, "User App", 8, 3);
    createProcess(&p[2], 3, "File Manager", 3, 2);

    printf("\n=== BEFORE SCHEDULING ===\n");

    for (int i = 0; i < 3; i++) {
        showProcess(p[i]);
    }

    printf("\n=== FCFS SCHEDULING ===\n");
    fcfs(p, 3);

    printf("\n=== SJF SCHEDULING ===\n");
    sjf(p, 3);

    printf("\n=== PRIORITY SCHEDULING ===\n");
    priority_scheduling(p, 3);

    return 0;
}