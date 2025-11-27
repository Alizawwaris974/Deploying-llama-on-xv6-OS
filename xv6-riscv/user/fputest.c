// xv6-riscv/user/fputest.c
// FPU Context Switch Test - Tests that FPU state is properly saved/restored across fork()

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void print_float(float val) {
  int intp = (int)val;
  int frac = (int)((val - intp) * 1000);
  if (frac < 0) frac = -frac;
  if (frac < 10) printf("%d.00%d", intp, frac);
  else if (frac < 100) printf("%d.0%d", intp, frac);
  else printf("%d.%d", intp, frac);
}

// Calculate expected child sum (cubes from 1.0 to 50.0, step 0.5)
// This verifies the child computed correctly, not using parent's FPU state
int calculate_expected_child(void) {
  float expected = 0.0;
  float j;
  for(j = 1.0; j <= 50.0; j += 0.5) {
    expected += j * j * j;
  }
  return (int)expected;
}

// Calculate expected parent sum (squares from 1.0 to 100.0, step 0.5)
int calculate_expected_parent(void) {
  float expected = 0.0;
  float i;
  for(i = 1.0; i <= 100.0; i += 0.5) {
    expected += i * i;
  }
  return (int)expected;
}

int
main(int argc, char *argv[])
{
  printf("FPU Context Switch Test\n");
  printf("========================\n\n");
  
  // Parent starts computation: sum of i^2 from 1.0 to 100.0, step 0.5
  printf("Parent: Starting computation (i^2 from 1.0 to 100.0)...\n");
  
  float parent_sum1 = 0.0;
  float i;
  
  // First half: 1.0 to 50.0
  for(i = 1.0; i <= 50.0; i += 0.5) {
    parent_sum1 += i * i;
  }
  
  printf("Parent: Completed first half, sum = %d (before fork)\n", (int)parent_sum1);
  
  // Fork child process
  int pid = fork();
  
  if(pid < 0) {
    printf("Fork failed\n");
    exit(1);
  }
  
  if(pid == 0) {
    // ==================== CHILD PROCESS ====================
    printf("\nChild: Starting computation (i^3 from 1.0 to 50.0)...\n");
    
    float child_sum = 0.0;
    float j;
    
    // Compute cubes from 1.0 to 50.0, step 0.5
    for(j = 1.0; j <= 50.0; j += 0.5) {
      child_sum += j * j * j;
    }
    
    printf("Child: Completed computation, sum = %d\n", (int)child_sum);
    
    // Calculate expected value automatically
    int expected = calculate_expected_child();
    int actual = (int)child_sum;
    int diff = actual > expected ? actual - expected : expected - actual;
    
    printf("Child: Expected = %d\n", expected);
    printf("Child: Actual   = %d\n", actual);
    printf("Child: Diff     = %d\n", diff);
    
    // Check if result matches expected (within 1% tolerance for floating point errors)
    if(diff < expected / 100) {
      printf("Child: PASS ✓ - FPU state correctly isolated from parent!\n\n");
      exit(0);  // Success
    } else {
      printf("Child: FAIL ✗ - Result incorrect (FPU state corruption?)\n\n");
      exit(1);  // Failure
    }
  } 
  else {
    // ==================== PARENT PROCESS ====================
    printf("\nParent: Waiting for child to complete...\n");
    
    // Wait for child to finish
    int child_status;
    wait(&child_status);
    
    printf("Parent: Child finished, continuing computation...\n");
    printf("Parent: Continuing computation (i^2 from 50.5 to 100.0)...\n");
    
    float parent_sum2 = 0.0;
    
    // Second half: 50.5 to 100.0
    for(i = 50.5; i <= 100.0; i += 0.5) {
      parent_sum2 += i * i;
    }
    
    float parent_total = parent_sum1 + parent_sum2;
    
    printf("\nParent: Completed computation\n");
    printf("Parent: First half sum  = %d\n", (int)parent_sum1);
    printf("Parent: Second half sum = %d\n", (int)parent_sum2);
    printf("Parent: Total sum       = %d\n", (int)parent_total);
    
    // Calculate expected value automatically
    int expected = calculate_expected_parent();
    int actual = (int)parent_total;
    int diff = actual > expected ? actual - expected : expected - actual;
    
    printf("Parent: Expected = %d\n", expected);
    printf("Parent: Actual   = %d\n", actual);
    printf("Parent: Diff     = %d\n", diff);
    
    // Check if result matches expected (within 1% tolerance)
    if(diff < expected / 100) {
      printf("Parent: PASS ✓ - FPU state correctly preserved across context switch!\n");
    } else {
      printf("Parent: FAIL ✗ - Result incorrect (FPU corruption?)\n");
    }
    
    printf("\n========================\n");
    
    // Overall test result
    if(child_status == 0 && diff < expected / 100) {
      printf("FPU Test: ALL TESTS PASSED ✓\n");
      printf("- FPU context switching works correctly\n");
      printf("- Child process has isolated FPU state\n");
      printf("- Parent FPU state preserved after fork\n");
    } else {
      printf("FPU Test: SOME TESTS FAILED ✗\n");
      if(child_status != 0) {
        printf("- Child test failed\n");
      }
      if(diff >= expected / 100) {
        printf("- Parent test failed\n");
      }
    }
    printf("========================\n");
  }
  
  exit(0);
}
