#include <stdio.h>
#include "eduos.h"

void fcfs(Process p[], int n);

int main() {

    Process p[3];

    createProcess(&p[0], 1, "System Idle", 5, 1);
    createProcess(&p[1], 2, "User App", 8, 3);
    createProcess(&p[2], 3, "File Manager", 3, 2);

    printf("=== BEFORE SCHEDULING ===\n");

    for (int i = 0; i < 3; i++) {
        showProcess(p[i]);
    }

    fcfs(p, 3);

    printf("\n=== AFTER SCHEDULING ===\n");

    for (int i = 0; i < 3; i++) {
        showProcess(p[i]);
    }

    return 0;
}