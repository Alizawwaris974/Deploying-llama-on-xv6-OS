#include "kernel/types.h"
#include "user/user.h"
#include "user/test_milestone2.h"

int main(int argc, char *argv[]) {
    printf("Starting Milestone 2 Tests...\n");
    
    test_math();
    test_string();
    test_stdlib();
    
    printf("All tests finished.\n");
    exit(0);
}
