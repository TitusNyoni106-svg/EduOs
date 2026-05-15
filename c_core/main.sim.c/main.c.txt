#include <stdio.h>
#include "eduos.h"


void createProcess(Process *p, int pid, const char name[]);
void showProcess(Process p);

int main() {
    Process p1;

    
    createProcess(&p1, 1, "System Idle");

    
    showProcess(p1);

    return 0;
}