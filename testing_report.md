# Baseline Benchmarking Report

## 1. Methodology

### Instrumentation
The code was instrumented using the `rdcycle` system call (mapped to the RISC-V `rdtime` instruction) to measure the number of CPU cycles consumed by specific code sections.
-   **Global Counters**: We introduced global accumulators (`g_matmul_cycles`, `g_attention_cycles`, etc.) to track time spent in hotspots across all layers and iterations.
-   **Timing Wrappers**: We wrapped critical functions (e.g., `matmul`, `transformer_step`) with start/end cycle captures.
    ```c
    long start = rdcycle();
    // ... operation ...
    long end = rdcycle();
    g_matmul_cycles += (end - start);
    ```
-   **Percentage Calculation**: Percentages are calculated as `(Component Cycles / Total Inference Cycles) * 100`. Note that since `matmul` is called *within* Attention and FFN blocks, the sum of percentages may exceed 100% if not carefully separated. In our reporting:
    -   `matmul` includes ALL matrix multiplications (Q, K, V, Output, FFN, Classifier).
    -   `Attention` includes the overhead of the attention mechanism (RoPE, Softmax) plus the matmuls within it.
    -   `FFN` includes the overhead of the Feed-Forward Network (SiLU, element-wise ops) plus its matmuls.

### OpenMP Removal Locations
The following locations contained `#pragma omp` directives in the original code and are identified as targets for parallelization in Milestone 5:
1.  **`matmul`**: The inner loops of the matrix multiplication function.
2.  **`transformer` (Attention)**: The loop over attention heads (`for (h = 0; h < p->n_heads; h++)`).
3.  **`transformer` (FFN)**: The matrix multiplications within the Feed-Forward Network.
4.  **`transformer` (Output)**: The final classifier matrix multiplication.

## 2. Performance Summary Table

| Test | Prompt Tokens | Output Tokens | TTFT (cycles) | TPS (tok/sec) | E2E Latency (s) | Top Hotspot (%) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **T1** | 5 | 46 | 17,265,074 | 2.79 | 17.87 | matmul (95.0%) |
| **T2** | 5 | 96 | 24,285,580 | 2.67 | 37.99 | matmul (94.4%) |
| **T3** | 41 | 10 | 142,338,590 | 2.86 | 17.39 | matmul (95.2%) |
| **T4** | 47 | 54 | 165,814,488 | 2.76 | 35.76 | matmul (94.4%) |

## 3. Computational Hotspot Analysis

| Component | T1 (%) | T2 (%) | T3 (%) | T4 (%) | AVG (%) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **matmul()** | 95.02 | 94.44 | 95.22 | 94.42 | **94.78** |
| **FFN** | 26.60 | 26.23 | 25.97 | 26.04 | 26.21 |
| **Attention** | 15.90 | 16.41 | 15.44 | 16.29 | 16.01 |
| **Activations** | 0.74 | 0.85 | 0.75 | 0.83 | 0.79 |
| **Sampling** | 0.10 | 0.10 | 0.02 | 0.06 | 0.07 |

*Note: `FFN` and `Attention` percentages include the time spent in their internal `matmul` calls. The global `matmul` percentage aggregates all matrix multiplications.*

## 4. Analysis

### Bottleneck Identification
The **`matmul`** function is the overwhelming bottleneck, consuming approximately **95%** of the total inference time. This is consistent across all test cases. The high percentage indicates that the model is compute-bound by matrix multiplications, specifically:
1.  **Q, K, V Projections** in Attention.
2.  **FFN Layers** (w1, w2, w3).
3.  **Output Classifier**: A large projection (288 -> 32000) at the end of every step.

### Scaling Behavior
-   **TTFT vs Prompt Length**: Time To First Token increases significantly with prompt length (T1: 1.7s vs T3: 14.2s). This is linear with the number of prompt tokens, as the prefill phase processes each token sequentially.
-   **TPS vs Output Length**: Tokens Per Second remains stable (~2.7 - 2.8) regardless of the number of output tokens generated. This confirms that the generation phase has a constant cost per token.

### Speedup Potential (Amdahl's Law)
Given that `matmul` constitutes ~95% of the execution time ($P = 0.95$):
-   If we parallelize `matmul` perfectly with $N=2$ threads:
    $$Speedup = \frac{1}{(1-0.95) + \frac{0.95}{2}} = \frac{1}{0.05 + 0.475} \approx 1.9x$$
-   If we parallelize `matmul` perfectly with $N=3$ threads (xv6 default CPUs):
    $$Speedup = \frac{1}{(1-0.95) + \frac{0.95}{3}} = \frac{1}{0.05 + 0.316} \approx 2.7x$$

This suggests highly promising returns for parallelization in Milestone 5.
