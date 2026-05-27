#ifndef EDUOS_H
#define EDUOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ================= PROCESS STRUCT =================
typedef struct {
    int pid;
    char name[50];
    char state[20];
    int burst_time;
    int priority;
} Process;

// ================= FUNCTION DECLARATIONS =================
void createProcess(Process *p, int pid, const char name[], int burst, int priority);
void showProcess(Process p);

void fcfs(Process p[], int n);
void sjf(Process p[], int n);
void priority_scheduling(Process p[], int n);

#endif