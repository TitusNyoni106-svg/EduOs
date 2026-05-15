#include <stdio.h>
#include <string.h>
#include "eduos.h"


void createProcess(Process *p, int pid, const char name[]) {
    p->pid = pid;
    strcpy(p->name, name);
    p->state = 1; // running
}


void showProcess(Process p) {
    printf("PID: %d\n", p.pid);
    printf("Name: %s\n", p.name);
    printf("State: %s\n", p.state == 1 ? "Running" : "Stopped");
}