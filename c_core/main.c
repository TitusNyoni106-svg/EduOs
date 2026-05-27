#include <stdio.h>
#include "eduos.h"

int main()
{
    Process p[3];

    // Create processes
    createProcess(&p[0], 1, "System Idle", 5, 1);
    createProcess(&p[1], 2, "User App", 8, 3);
    createProcess(&p[2], 3, "File Manager", 3, 2);

    // ================= SYSTEM CALLS =================

    edu_fork(&p[0], &p[1], 4);

    edu_exec(&p[1], "New User App");

    edu_wait(&p[1]);

    edu_exit(&p[2]);

    // ================= PROCESS LIST =================

    edu_ps(p);

    printf("\n=== BEFORE SCHEDULING ===\n");

    for (int i = 0; i < 3; i++) {
        showProcess(p[i]);
    }

    // ================= SCHEDULERS =================

    fcfs(p, 3);

    sjf(p, 3);

    priority_scheduling(p, 3);

    // ================= FINAL STATE =================

    printf("\n=== AFTER SCHEDULING ===\n");

    for (int i = 0; i < 3; i++) {
        showProcess(p[i]);
    }

    return 0;
}