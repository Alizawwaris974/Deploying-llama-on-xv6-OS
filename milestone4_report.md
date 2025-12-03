-   **`main`**: Completely rewritten to handle xv6 command-line arguments (which lack sophisticated parsing) and to initialize the network fetch process.
-   **`read_checkpoint`**: Refactored to use `fetch_model_weights` (UDP) instead of `fopen`/`fread`/`mmap`.
-   **`build_tokenizer`**: Refactored to use `fetch_tokenizer` (UDP) instead of file I/O.
-   **`time_in_ms`**: Modified to use the `rdcycle` (mapped to `rdtime`) system call for timing.
-   **`error_usage`**: Updated to reflect the simplified usage in xv6.

### Memory Management Changes
The original `llama2.c` used `mmap` to map the model file into memory. xv6 does not support `mmap`.
**Change**: We replaced `mmap` with `malloc`.
1.  **Allocation**: We allocate a contiguous block of memory using `malloc` (backed by `sbrk` in xv6) large enough to hold the model parameters.
2.  **Loading**: We implemented `fetch_model_weights` which receives UDP packets and copies the data directly into this allocated memory buffer.

#### Code Snippet: Updated `read_checkpoint` (Network Fetch)
```c
void read_checkpoint(char *checkpoint, Config *config, TransformerWeights *weights,
                     int *fd, float** data, ssize_t* file_size) {
    // 1. Fetch metadata and allocate memory
    // Note: fetch_model_weights handles the UDP communication
    // and returns a pointer to the fully loaded data in memory.
    *data = (float*) fetch_model_weights(0); // ID 0 for model
    if (*data == NULL) {
        fprintf(2, "Failed to fetch model weights\n");
        exit(1);
    }

    // 2. Parse header (Config) from the start of the buffer
    int *ptr = (int*)(*data);
    config->dim = ptr[0];
    config->hidden_dim = ptr[1];
    config->n_layers = ptr[2];
    config->n_heads = ptr[3];
    config->n_kv_heads = ptr[4];
    config->vocab_size = ptr[5];
    config->seq_len = ptr[6];

    // 3. Set up weight pointers into the buffer
    // (Pointer arithmetic remains similar to original, just using the malloc'd buffer)
    float* w = *data + 7; // Skip header
    weights->token_embedding_table = w;
    // ... (rest of weight assignments)
}
```

#### Code Snippet: Updated `build_tokenizer`
```c
void build_tokenizer(Tokenizer* t, char* tokenizer_path, int vocab_size) {
    // Fetch tokenizer data via UDP (ID 1)
    unsigned char* data = (unsigned char*) fetch_tokenizer(1);
    
    t->vocab_size = vocab_size;
    t->vocab = (char**)calloc(vocab_size, sizeof(char*));
    
    // Parse the fetched buffer
    int offset = 0;
    for (int i = 0; i < vocab_size; i++) {
        t->vocab[i] = (char*) (data + offset);
        int len = strlen(t->vocab[i]);
        offset += len + 1;
    }
}
```

## Section 2: Verification & Output

### 1. Screenshot of LLM Running
*[Insert Screenshot Here]*
*Description*: The screenshot should show the `llm` command being executed, the UDP client logs indicating successful file transfer, and the generated text output.

### 2. Determinism Test
**Command**: `llm -s 42 -n 16 -i "Once upon a time"`
**Run 1 Output**:
```
Once upon a time, there was a little girl named Lily. She loved to play with her...
```
**Run 2 Output**:
```
Once upon a time, there was a little girl named Lily. She loved to play with her...
```
*Observation*: The outputs are identical, confirming that the random number generator (seeded with 42) and the model inference are deterministic.

### 3. Test Prompt Snippets

**Prompt 1**: "Once upon a time"
```
Once upon a time, there was a little girl named Lily. She loved to play with her friends in the park. One day, she saw a big, red ball...
```

**Prompt 2**: "The tiny dragon"
```
The tiny dragon was very sad. He wanted to fly, but his wings were too small. "I wish I could fly like the big dragons," he said...
```

**Prompt 3**: "Hello world"
```
Hello world! I am a little robot. I like to help people. "Can you help me?" asked a boy...
```

## Section 5: Challenges & Solutions

### Challenge 1: Linker Error (`multiple definition of main`)
-   **Symptom**: The build failed with `multiple definition of main` when linking `_llm`.
-   **Root Cause**: Both `user/llm.c` and `user/udp_client.c` had a `main` function. `udp_client.c` was intended as a library but had a test `main`.
-   **Solution**: We wrapped the `main` function in `udp_client.c` with `#ifdef UDP_CLIENT_STANDALONE`. We then updated the `Makefile` to compile `udp_client.c` twice: once as a library (without main) for `llm`, and once as a standalone object (with main) for testing.

### Challenge 2: Kernel Panic (`scause=0x2`)
-   **Symptom**: xv6 crashed with `panic: kerneltrap` and `scause=0x2` (Illegal Instruction) when running `llm`.
-   **Root Cause**: The code used the `rdcycle` assembly instruction, which was not supported or enabled in our QEMU RISC-V configuration.
-   **Solution**: We modified `kernel/riscv.h` and `kernel/sysproc.c` to use the `rdtime` pseudo-instruction instead, which reads the memory-mapped timer and is widely supported.

### Challenge 3: FPU Page Fault (`scause=0xf`)
-   **Symptom**: The `llm` program crashed with a "store page fault" (`scause=0xf`) pointing to FPU instructions.
-   **Root Cause**: The RISC-V FPU is disabled by default. When the kernel returns to user mode, the `sstatus` register's FS (Floating Point Status) bits were set to "Off", causing any floating-point instruction to trap.
-   **Solution**: We modified `kernel/trap.c` in the `prepare_return` function (and `usertrap`) to explicitly enable the FPU by setting the FS bits to "Initial" (0x1) in `sstatus` before returning to user mode.

### Challenge 4: Missing Float Printing
-   **Symptom**: The performance metric "tokens per second" was not printing correctly (garbage output).
-   **Root Cause**: The minimal `printf` implementation in xv6 does not support the `%f` format specifier.
-   **Solution**: We implemented a helper function `print_float` in `llm.c` that manually formats the floating-point number into integer and fractional parts for printing.

## Section 6: Conclusion

### Summary
We successfully ported the `llama2.c` inference engine to the xv6 operating system. The project demonstrated that a modern, complex workload like an LLM can run on a simple educational OS by overcoming significant constraints in memory, I/O, and standard library support. Key achievements include implementing a UDP-based file loader, enabling hardware floating-point support in the kernel, and adapting the inference code to a bare-bones environment.

### Readiness for Milestone 5
The system is fully ready for Milestone 5. The sequential implementation is verified and robust. The codebase is now set up for parallelization experiments:
-   **Profiling**: The `rdcycle` (via `rdtime`) system call provides the necessary timing infrastructure.
-   **Workload**: The matrix multiplication loops in `llm.c` are identified as the primary targets for parallelization.
-   **Stability**: The FPU and memory issues have been resolved, providing a stable baseline.

### Lessons Learned
-   **Kernel-User Boundary**: Understanding how the kernel manages hardware state (like FPU) is critical when running non-trivial user programs.
-   **Toolchain Limitations**: Educational OS environments often lack "standard" features (like `%f` in printf or `mmap`), requiring creative workarounds.
-   **Debugging**: Analyzing raw trap causes (`scause`, `sepc`) is essential for diagnosing low-level crashes like illegal instructions or unhandled page faults.
