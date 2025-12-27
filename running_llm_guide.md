# Running the Parallelized LLM in xv6

This guide explains how to run the multithreaded LLM inference in xv6, including setting up the file server for model weights.

## Prerequisites

1.  **Python Environment**: Ensure you have Python 3 installed on your host machine (WSL/Linux/macOS).
2.  **Model Files**: You need `stories15M.bin` and `tokenizer.bin` in your project root.
    *   If you don't have them, download them from the course resources or the `karpathy/llama2.c` repository.
3.  **Network Enabled xv6**: The build must support networking (usually `LAB=net` or similar configuration in Makefile, but our Makefile seems to handle it).

## Step 1: Start the File Server

Since the model file (~60MB) is too large to embed directly into the xv6 filesystem image easily, we fetch it over UDP from the host.

Open a terminal on your **host machine** (outside QEMU) and run:

```bash
# Navigate to project root
cd /path/to/OS-Fall25-Project-Interstellar

# Run the server (default port 9999)
# Usage: python3 file_server.py <model_path> <tokenizer_path>
python3 file_server.py stories15M.bin tokenizer.bin
```

You should see:
```
Server listening on 0.0.0.0:9999
Waiting for requests from xv6...
```

## Step 2: boot xv6 with Multithreading Support

Open a **separate terminal** and boot xv6. We explicitly set 3 CPUs to ensure one main thread and two worker threads can run in parallel.

```bash
make qemu CPUS=3
```

## Step 3: Run the LLM

Inside the xv6 shell, run the `llm` command. Note that the model filename is passed as a positional argument.

```bash
# Syntax: llm <model_filename> -t <tokenizer_filename> -i <prompt>

llm llama2_15m.bin -t tokenizer.bin -i "Once upon a time"
```

### What happens next?
1.  **Downloading**: The client will connect to your Python server and download the model/tokenizer chunks. You will see progress bars in your Python server terminal.
2.  **Inference**: Once downloaded, the model initializes the thread pool (2 workers + 1 main).
3.  **Output**: You will see the generated text token by token.
4.  **Metrics**: At the end, performance metrics (Cycles, Tokens/Sec) will be displayed.

## Troubleshooting

*   **Network Error/Hang**: Ensure QEMU is running with the correct network flags. If using the standard Makefile provided, it should set up user networking forwarding ports.
*   **Panic `freewalk`**: If you see a kernel panic, ensure you are running the latest code from `milestone-5` which contains fixes for thread cleanup.
