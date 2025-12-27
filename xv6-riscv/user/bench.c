#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Test config structure
struct test_config {
    char *name;
    char *prompt;
    char *steps; // "-n" value
};

struct test_config tests[] = {
    // T1: Baseline (Short prompt, 50 tokens)
    {"T1", "Once upon a time", "50"},
    // T2: Output Scaling (Short prompt, 100 tokens)
    {"T2", "Once upon a time", "100"},
    // T3: Prompt Scaling (Longer prompt, 50 tokens)
    {"T3", "The tiny dragon was very sad. He wanted to fly, but his wings were too small. I wish I could fly like the big dragons, he climbed the highest mountain to practice.", "50"},
    // T4: Combined (Longer prompt, 100 tokens)
    {"T4", "The tiny dragon was very sad. He wanted to fly, but his wings were too small. I wish I could fly like the big dragons, he said. One day, he climbed the highest mountain to practice.", "100"}
};

void run_test(struct test_config *t) {
    int pid;
    
    printf("\n=== RUNNING BENCHMARK: %s ===\n", t->name);
    printf("Prompt: \"%s\"\n", t->prompt);
    printf("Steps: %s\n", t->steps);
    
    pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }
    
    if (pid == 0) {
        // Child process: execute llm
        // args: llm <model> -t <tokenizer> -i <prompt> -n <steps>
        char *argv[] = {
            "llm",
            "llama2_15m.bin",
            "-t", "tokenizer.bin",
            "-i", t->prompt,
            "-n", t->steps,
            0
        };
        
        exec("llm", argv);
        printf("exec failed\n");
        exit(1);
    } else {
        // Parent process: wait for child
        wait(0);
        printf("=== FINISHED %s ===\n", t->name);
    }
}

int main(int argc, char *argv[]) {
    int i;
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    printf("Starting Automated Benchmarks (Milestone 5)...\n");
    // Ensure thread pool is creating 2 workers + 1 main = 3 threads
    // This is handled inside llm.c, assuming CPUS=3 in Makefile
    
    for (i = 0; i < num_tests; i++) {
        run_test(&tests[i]);
        // Small delay between tests to let buffers/cleanup settle if needed
        pause(10); 
    }
    
    printf("\nAll benchmarks completed.\n");
    exit(0);
}
