#include <stdio.h>
#include "eduos.h"

void createProcess(Process *p, int pid, const char name[]);
void showProcess(Process p);
void stopProcess(Process *p);



int main() {

    Process p1, p2, p3;

    createProcess(&p1, 1, "System Idle");
    createProcess(&p2, 2, "User App");
    createProcess(&p3, 3, "File Manager");
     
    stopProcess(&p2);

    showProcess(p1);
    showProcess(p2);
    showProcess(p3);

    return 0;
}