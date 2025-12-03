#include "kernel/types.h"
#include "user/user.h"

// Simple test program to demonstrate TTFT and TPS metrics
// This simulates the LLM generation loop without requiring actual model weights

#define CPU_FREQ 10000000.0

void print_float(float f) {
    if (f != f) { printf("NaN"); return; }
    if (f > 1e30f) { printf("Inf"); return; }
    if (f < -1e30f) { printf("-Inf"); return; }
    
    if (f < 0) {
        printf("-");
        f = -f;
    }
    int int_part = (int)f;
    int frac_part = (int)((f - int_part) * 1000000); // 6 decimal places
    if (frac_part < 0) frac_part = 0;
    
    printf("%d.", int_part);
    // Manual padding
    if (frac_part < 100000) printf("0");
    if (frac_part < 10000) printf("0");
    if (frac_part < 1000) printf("0");
    if (frac_part < 100) printf("0");
    if (frac_part < 10) printf("0");
    printf("%d", frac_part);
}

// Simulate some computation work
void do_work(int iterations) {
    volatile int sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += i * i;
    }
}

int main(int argc, char *argv[]) {
    int num_tokens = 10;  // Default number of tokens to generate
    int work_per_token = 100000;  // Computation per token
    
    if (argc > 1) {
        num_tokens = atoi(argv[1]);
    }
    
    printf(" TTFT and TPS Metrics Test \n");
    printf("Simulating generation of %d tokens\n\n", num_tokens);
    
    // Simulate prompt processing + first token generation
    printf("Processing prompt and generating tokens \n");
    
    // METRIC TRACKING - START
    long start_time = rdcycle();
    long first_token_time = 0;
    long end_time = 0;
    int generated_tokens = 0;
    
    // Simulate prompt processing (heavier work for first token)
    printf("[Prompt processing]");
    do_work(work_per_token * 3);  // Prompt processing is heavier
    
    // Generate tokens
    for (int i = 0; i < num_tokens; i++) {
        // Simulate token generation
        do_work(work_per_token);
        generated_tokens++;
        
        // Capture time when first token is generated
        if (first_token_time == 0) {
            first_token_time = rdcycle();
        }
        
        printf(" T%d", i + 1);
    }
    printf("\n\n");
    
    // Capture end time
    end_time = rdcycle();
    
    // METRIC REPORTING
    if (generated_tokens > 0) {
        // TTFT: cycles from start to first generated token
        long ttft_cycles = first_token_time - start_time;
        printf("Performance Metrics:  \n");
        printf("Time to First Token (TTFT): %ld cycles\n", ttft_cycles);
        
        // TPS: tokens per second for generation phase (excluding first token)
        if (generated_tokens > 1) {
            long generation_cycles = end_time - first_token_time;
            double generation_time_seconds = generation_cycles / CPU_FREQ;
            double tps = (generated_tokens - 1) / generation_time_seconds;
            printf("Tokens Per Second (TPS): ");
            print_float(tps);
            printf("\n");
            printf("Total generated tokens: %d\n", generated_tokens);
            printf("Generation time: %ld cycles\n", generation_cycles);
            printf("Total time: %ld cycles\n", end_time - start_time);
        } else {
            printf("Tokens Per Second (TPS): N/A (only 1 token generated)\n");
        }
        printf("\n");
    }
    
    printf("\nTest completed successfully \n");
    exit(0);
}
