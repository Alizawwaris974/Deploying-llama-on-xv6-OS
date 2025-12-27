#include "kernel/types.h"
#include "user/user.h"

#define NUM_TESTS 4

struct TestConfig {
    char name[10];
    char prompt[256];
    int output_tokens;
};

struct TestConfig tests[NUM_TESTS] = {
    {"T1", "Once upon a time", 50},
    {"T2", "Once upon a time", 100},
    {"T3", "The quick brown fox jumps over the lazy dog.", 50},
    {"T4", "The quick brown fox jumps over the lazy dog.", 100}
};

// Simple busy wait instead of sleep
void delay(int iterations) {
    volatile int counter = 0;
    for (int i = 0; i < iterations; i++) {
        counter++;
    }
}

// Convert int to string for xv6
void int_to_string(int n, char* buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    int i = 0;
    int is_negative = 0;
    
    if (n < 0) {
        is_negative = 1;
        n = -n;
    }
    
    // Extract digits in reverse
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    if (is_negative) {
        buf[i++] = '-';
    }
    
    buf[i] = '\0';
    
    // Reverse the string
    int len = i;
    for (int j = 0; j < len / 2; j++) {
        char temp = buf[j];
        buf[j] = buf[len - 1 - j];
        buf[len - 1 - j] = temp;
    }
}

void run_benchmark(int thread_count, struct TestConfig* test) {
    printf("\n================================\n");
    printf("Running: %s with %d threads\n", test->name, thread_count);
    printf("Prompt: %s\n", test->prompt);
    printf("Output tokens: %d\n", test->output_tokens);
    printf("================================\n");
    
    // Convert numbers to strings
    char token_str[16];
    char thread_str[16];
    int_to_string(test->output_tokens, token_str);
    int_to_string(thread_count, thread_str);
    
    // Create args for exec
    char* args[] = {
        "llm",
        "llama2_15m.bin",
        "-t", "tokenizer.bin",
        "-i", test->prompt,
        "-n", token_str,
        "-s", "12345",
        "-temp", "0.0",
        "-b", thread_str,
        "-c", test->name,
        0
    };
    
    int pid = fork();
    if (pid == 0) {
        // Child process - run the benchmark
        exec("llm", args);
        printf("exec failed!\n");
        exit(1);
    } else {
        wait(0);  // Wait for completion
    }
    
    // Small delay between tests
    delay(100000);
}

int main(int argc, char *argv[]) {
    printf("\n");
    printf("╔══════════════════════════════════════╗\n");
    printf("║      LLM Benchmark Runner (xv6)      ║\n");
    printf("╚══════════════════════════════════════╝\n\n");
    
    printf("Running %d test cases × 3 thread counts\n", NUM_TESTS);
    printf("Total: %d benchmark runs\n\n", NUM_TESTS * 3);
    
    // Run with 1, 2, and 3 threads
    int total_runs = 0;
    for (int threads = 1; threads <= 3; threads++) {
        printf("\n■■■■■ THREADS: %d ■■■■■\n", threads);
        
        for (int i = 0; i < NUM_TESTS; i++) {
            total_runs++;
            printf("\n[%d/%d] %s\n", total_runs, NUM_TESTS * 3, tests[i].name);
            run_benchmark(threads, &tests[i]);
        }
    }
    
    printf("\n════════════════════════════════\n");
    printf("✓ All benchmarks completed!\n");
    printf("  Run 'analyze' to view results.\n");
    printf("════════════════════════════════\n");
    
    exit(0);
}