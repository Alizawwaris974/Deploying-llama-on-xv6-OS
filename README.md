# Deploying Llama on xv6 OS

## Team Information
- **Team Name:** OS-Fall25-Project-Interstellar
- **Team Members:**
  - Hammadullah 
  - Aleena Meraj 
  - Abdur Rehman
  - Aliza M Warris 

## Project Overview
This project focuses on deploying the Llama2 inference engine on the xv6 operating system. The implementation spans three milestones, each building upon the previous one to achieve efficient LLM inference in a lightweight OS environment.

## Milestone 1: Foundation & Analysis
**Deadline:** 27 October 2025

### Goals
- Analyze the LLM inference code through profiling
- Conduct initial research on FPU implementation and math functions
- Plan for xv6 integration

### Key Tasks
1. **Repository Setup**
   - Create private GitHub repository
   - Add all team members and instructors as collaborators
   - Clone llama2.c into llama2c/ subfolder

2. **FPU & Math Functions Literature Review**
   - Investigate floating-point number management at OS level
   - Focus on RISC-V architecture (F and D extensions)
   - Identify key CSRs: fcsr, frm, fflags
   - Research implementation methods: CORDIC, Taylor series, lookup tables
   - Analyze three functions: exp, sqrt, tanh

3. **llama2.c Analysis**
   - Compile and run llama2.c on host machine
   - Use profiling tools (perf) to identify bottlenecks
   - Run with different parameter sizes and note execution times

4. **xv6 Dependency Assessment**
   - Compare llama2.c requirements with xv6 features
   - Create function dependency graph
   - Identify missing dependencies and required modifications

### Resources
- LLM Videos: https://youtu.be/wjZofJX0v4M, https://youtu.be/eMlx5fFNoYc, https://youtu.be/9-Jl0dxWQs8
- Llama2.c: https://github.com/karpathy/llama2.c

### Deliverables
- TeamName_Report_Milestone1.pdf (IEEE format recommended)
- GitHub repository with llama2c folder

### Grading Criteria (100 points)
- Repository Setup: 10 points
- FPU & Math Functions Review: 40 points
- Code Analysis & xv6 Assessment: 40 points
- Report Format and Structure: 10 points

---

## Milestone 2: Core Dependencies & Libraries
**Deadline:** 9 November 2025

### Goals
- Establish foundational infrastructure for LLM inference
- Implement floating-point support at kernel level
- Create user-level standard library functions

### Key Tasks
1. **Repository Setup & Base Code Integration**
   - Create milestone-2 branch from milestone-1
   - Clone provided xv6-riscv repository
   - All work on milestone-2 branch

2. **Floating-Point Unit (FPU) Support (Kernel-Level)**
   - Enable FPU at boot by setting RISC-V CSRs
   - Modify context switching to save/restore FP registers (f0-f31)
   - Handle FPU traps and exceptions
   - Create fputest.c to validate multi-process FPU usage

3. **Math Library Functions (User-Level)**
   - Implement: sqrtf, expf, powf, cosf, sinf, fabsf
   - Files: user/xv6_math.c and user/xv6_math.h
   - Target accuracy: within 1e-5 for single-precision floats
   - Use algorithms: Taylor series, CORDIC, Chebyshev polynomials

4. **String Functions (User-Level)**
   - Implement: memcpy, memset, strcmp, strlen, sprintf, strcpy, sscanf, isprint, isspace
   - Files: user/xv6_string.c and user/xv6_string.h
   - Support sprintf format specifiers: %d, %s, %f, %c, %%

5. **Standard Library Functions (User-Level)**
   - Implement: calloc, bsearch, qsort, atoi, atof
   - Files: user/xv6_stdlib.c and user/xv6_stdlib.h
   - Handle edge cases and error conditions

6. **Comprehensive Testing & Validation**
   - Create: test_math.c, test_string.c, test_stdlib.c
   - Master test runner: test_milestone2.c
   - Shared header: test_milestone2.h
   - Generate 50-100 test cases per math function
   - Track: passes, failures, max error, average error

### Resources
- Mathematical Functions: https://www.youtube.com/watch?v=p8u_k2LIZyo, https://github.com/nadavrot/fast_log
- RISC-V ISA: https://drive.google.com/file/d/1uviu1nH-tScFfgrovvFCrj7Omv8tFtkp/view
- Privileged ISA: https://drive.google.com/file/d/17GeetSnT5wW3xNuAHI95-SI1gPGd5sJ_/view

### Deliverables
- TeamName_Report_Milestone2.pdf
- Source code on milestone-2 branch:
  - Kernel: proc.h, proc.c, swtch.S, start.c, main.c, trap.c (modified for FPU)
  - User: xv6_math.c/h, xv6_string.c/h, xv6_stdlib.c/h
  - Tests: fputest.c, test_math.c, test_string.c, test_stdlib.c, test_milestone2.c/h

### Grading Criteria (100 points)
- Repository Setup and Git Usage: 10 points
- FPU Support in xv6: 30 points
- Math Library Implementation: 20 points
- String Library Implementation: 12 points
- Standard Library Implementation: 10 points
- Testing & Validation: 10 points
- Report Quality: 8 points

---

## Milestone 3: Network-Based Weight Fetching
**Deadline:** 22 November 2025

### Goals
- Build xv6 network stack to fetch LLM weights
- Implement E1000 driver, SHA-256 library, and reliable file transfer protocol
- Enable runtime fetching and verification of model files (60+ MB)

### Key Tasks
1. **Repository Setup & System Configuration**
   - Create milestone-3 branch from milestone-2
   - Apply required code changes (conf/lab.mk, kernel/vm.c, Makefile)
   - Increase xv6 physical memory from 128 MB to 256+ MB

2. **SHA-256 Implementation (User-Level)**
   - Files: user/sha256.c and user/sha256.h
   - Public interface: void sha256_hash(const unsigned char *data, unsigned int len, unsigned char hash[32])
   - Test cases: empty string, single character, short string, block boundary, multi-block
   - All tests must pass with correct hashes

3. **xv6 Networking Stack (Kernel-Level)**

   **E1000 NIC Driver Implementation:**
   - Implement e1000_transmit() function
   - Implement e1000_recv() function
   - Manage transmit (TX) and receive (RX) descriptor rings
   - Handle DMA (direct memory access) operations
   - Pass txone and rxone tests

   **UDP Receive Implementation:**
   - Implement ip_rx() for UDP packet identification
   - Implement sys_bind() for port binding
   - Implement sys_recv() for packet queuing and retrieval
   - Support 16-packet queue per port
   - Handle byte order conversion (network to host)

4. **Reliable File Transfer Protocol Design**
   - Design protocol to handle packet loss, duplication, reordering
   - Verify integrity using SHA-256 checksums
   - Define message types and byte-level structure
   - Create sequence diagram for communication flow
   - Document design rationale

   **Message Format Specification:**
   - Client request types
   - Server response types
   - Packet structure and sequencing
   - Byte ordering (network byte order)

   **Suggested Patterns:**
   - Pattern 1: Simple Request-Response (metadata → packets → verification)
   - Pattern 2: TFTP-Inspired (numbered blocks with acknowledgments)

5. **Python File Server Implementation**
   - File: file_server.py (repository root)
   - Listen on UDP port 9999
   - Load files: stories15M.bin (~60 MB), tokenizer.bin
   - Pre-compute SHA-256 checksums
   - Parse requests and respond according to protocol
   - Use Python: socket, hashlib, struct libraries

6. **xv6 Client-Side File Transfer Library (User-Level)**
   - Files: user/udp_client.c and user/udp_client.h
   - Base protocol functions for file fetching
   - LLM wrapper functions:
     - char* fetch_model_weights(int *size_out)
     - char* fetch_tokenizer(int *size_out)
   - SHA-256 verification of transferred files
   - Error handling for all failure modes

7. **Optional Bonus: In-Memory Persistence**
   - Cache fetched model weights in memory
   - Allow access across multiple LLM executions
   - Prevent redundant downloads

### Resources
- Networking: https://pdos.csail.mit.edu/6.1810/2025/labs/net.html
- RFC 768 (UDP): https://datatracker.ietf.org/doc/html/rfc768
- QEMU Networking: https://wiki.qemu.org/Documentation/Networking
- E1000 Manual: https://pdos.csail.mit.edu/6.1810/2025/readings/8254x_GBe_SDM.pdf
- Networking Guide: https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf
- SHA-256: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
- TFTP: https://en.wikipedia.org/wiki/Trivial_File_Transfer_Protocol

### Deliverables
- TeamName_Report_Milestone3.pdf
- Source code on milestone-3 branch:
  - Kernel: e1000.c, e1000_dev.h, net.c, net.h, pci.c (modified)
  - User: sha256.c/h, udp_client.c/h
  - Server: file_server.py (root directory)
  - Config changes: conf/lab.mk, kernel/vm.c, Makefile

### Network Environment
- Guest IP: 10.0.2.15 (xv6)
- Host IP: 10.0.2.2 (QEMU user-mode network)
- Network emulation via QEMU's E1000 controller

### Performance Expectations
- File transfer (~60 MB) should complete in under 60 seconds

### Grading Criteria (100 points)
- Repository Setup & System Configuration: 5 points
- SHA-256 Implementation: 10 points
- xv6 Networking Stack: 30 points
  - E1000 Driver: 15 points
  - UDP Receive: 15 points
- Reliable File Transfer Protocol: 10 points
- Python File Server: 15 points
- xv6 Client Library: 15 points
- Report Quality: 15 points
- Bonus (In-Memory Persistence): up to 10 points

## Development Guidelines

### Git Workflow
- Each milestone has its own branch (milestone-1, milestone-2, milestone-3)
- Make logical commits per task
- Use descriptive commit messages
- Squash small commits before submission
- Do not mix code from different tasks

### Coding Standards
- Follow xv6 coding conventions
- Comment complex logic
- Handle edge cases
- Implement error checking
- Use xv6-compatible types and functions

### Testing Requirements
- All implementations must be thoroughly tested
- Include unit tests for all functions
- Provide test coverage evidence
- Document test results in reports

## Important Dates
- **Milestone 1 Deadline:** 27 October 2025
- **Milestone 2 Deadline:** 9 November 2025
- **Milestone 3 Deadline:** 22 November 2025

## Resources & References
- xv6 Book: https://pdos.csail.mit.edu/6.1810/
- RISC-V ISA Manual: https://riscv.org/
- IEEE LaTeX Templates: https://www.overleaf.com/

## Contact & Support
For questions or issues related to this project, please contact your instructor or TAs through the course management system.

## License
This project is part of OS Fall 2025 course at IBA.
