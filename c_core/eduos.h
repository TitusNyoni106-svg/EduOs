#ifndef EDUOS_H
#define EDUOS_H

#define OS_NAME "EduOS"
#define VERSION "0.1"

// Process states
#define READY 0
#define RUNNING 1
#define TERMINATED 2

typedef struct {
    int pid;
    char name[50];
    int state;
    int burst_time;
    int priority;
} Process;

void createProcess(Process *p, int pid, const char name[], int burst, int priority);
void showProcess(Process p);
void stopProcess(Process *p);
void fcfs(Process p[], int n);

#endif