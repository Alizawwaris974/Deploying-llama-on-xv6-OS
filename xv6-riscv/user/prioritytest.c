#include "kernel/types.h"
#include "user/user.h"

void churn(int p) {
    int i; 
    volatile int sum = 0;
    // CPU intensive loop
    for(i=0; i<100000000; i++) {
        sum += i;
        if(i % 10000000 == 0) {
           pause(1); // Yield briefly to let scheduler decide
        }
    }
    printf("Process with priority %d finished.\n", p);
    exit(0);
}

int main() {
    int pid1, pid2, pid3;

    printf("Starting priority test...\n");

    // Process 1: Low Priority (1)
    if((pid1 = fork()) == 0) {
        set_priority(1);
        printf("Started Low Priority Process (PID %d)\n", getpid());
        pause(20); 
        churn(1);
    }

    // Process 2: Medium Priority (10)
    if((pid2 = fork()) == 0) {
        set_priority(10);
        printf("Started Medium Priority Process (PID %d)\n", getpid());
        pause(20); 
        churn(10);
    }

    // Process 3: High Priority (20)
    if((pid3 = fork()) == 0) {
        set_priority(20);
        printf("Started High Priority Process (PID %d)\n", getpid());
        pause(20); 
        churn(20);
    }

    // Wait for all to finish
    int fini_count = 0;
    while(fini_count < 3) {
        wait(0);
        fini_count++;
    }
    
    printf("Priority test finished.\n");
    exit(0);
}
