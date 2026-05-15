#ifndef EDUOS_H
#define EDUOS_H

#define OS_NAME "EduOS"
#define VERSION "0.1"


typedef struct {
    int pid;
    char name[50];
    int state;   // 0 = stopped, 1 = running
} Process;


void createProcess(Process *p, int pid, const char name[]);
void showProcess(Process p);

#endif