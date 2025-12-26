#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>
#include <time.h>

#define N 1000

// =================================================================
// SECTION 1: BASIC SYNTAX & THREAD MANAGEMENT (Lab 02)
// =================================================================
void basic_threading() {
    printf("\n--- Basic Threading ---\n");
    
    // Setting thread count inside code (Alternative to export OMP_NUM_THREADS=4)
    omp_set_num_threads(4); 

    #pragma omp parallel 
    {
        int id = omp_get_thread_num();
        int total = omp_get_num_threads();
        printf("Hello from thread %d of %d\n", id, total);
    }
}

// =================================================================
// SECTION 2: WORK SHARING & SCHEDULES (Lab 03, Lab 06)
// =================================================================
void loop_scheduling() {
    printf("\n--- Loop Scheduling (Static vs Dynamic) ---\n");
    
    // STATIC: Good for equal work (Matrix Mult, Vector Add)
    // Chunks are fixed at compile time.
    #pragma omp parallel for schedule(static, 2)
    for (int i = 0; i < 10; i++) {
        // printf("Thread %d handling static %d\n", omp_get_thread_num(), i);
    }

    // DYNAMIC: Good for unequal work (Prime finding, While loops)
    // Threads steal work when finished.
    #pragma omp parallel for schedule(dynamic, 2)
    for (int i = 0; i < 10; i++) {
        // printf("Thread %d handling dynamic %d\n", omp_get_thread_num(), i);
    }
    
    // ORDERED: Forces output/execution to happen in sequential order
    // MUST use 'ordered' clause on loop AND 'ordered' directive inside
    #pragma omp parallel for ordered
    for (int i = 0; i < 5; i++) {
        #pragma omp ordered
        printf("Ordered output: %d by thread %d\n", i, omp_get_thread_num());
    }
}

// =================================================================
// SECTION 3: DATA ENVIRONMENT & SYNC (Lab 05)
// =================================================================
void synchronization_demo() {
    printf("\n--- Synchronization Constructs ---\n");
    int shared_sum = 0;

    #pragma omp parallel
    {
        // 1. MASTER: Only Thread 0 runs this. No implicit barrier.
        #pragma omp master
        {
            printf("Master thread initializing...\n");
        }

        // 2. BARRIER: Everyone waits here until all arrive
        #pragma omp barrier

        // 3. ATOMIC: Fast, safe single-memory update (x++, x+=y)
        #pragma omp atomic
        shared_sum += 1;

        // 4. CRITICAL: Safe block for complex code (I/O, multi-line)
        #pragma omp critical
        {
            // Only one thread prints at a time (prevents garbled text)
            // printf("Thread %d is in critical section\n", omp_get_thread_num());
        }
    }
    printf("Final Atomic Sum: %d\n", shared_sum);
}

// =================================================================
// EXAM QUESTION 1: MANUAL REDUCTION (Lab 05 Task 5 Logic)
// Question: "Sum an array in parallel without using the 'reduction' clause"
// =================================================================
void manual_reduction_exam() {
    printf("\n--- Exam Q1: Manual Reduction ---\n");
    int arr[20];
    for(int i=0; i<20; i++) arr[i] = 1;

    int total_sum = 0;

    #pragma omp parallel
    {
        int local_sum = 0; // PRIVATE by declaration inside scope

        // Distribute loop, NO reduction clause
        #pragma omp for
        for (int i = 0; i < 20; i++) {
            local_sum += arr[i];
        }

        // Safely add local result to global result
        #pragma omp atomic
        total_sum += local_sum;
    }
    printf("Manual Sum Result: %d\n", total_sum);
}

// =================================================================
// EXAM QUESTION 2: MATRIX MULTIPLICATION (Lab 04 Task 5)
// Question: "Parallelize Matrix Mult. Ensure no Race Conditions."
// =================================================================
void matrix_mult_exam() {
    printf("\n--- Exam Q2: Matrix Multiplication ---\n");
    int A[N][N], B[N][N], C[N][N];
    int i, j, k;

    // Initialize (omitted for brevity)

    // Parallelize OUTER loop
    // j, k, sum MUST be private. i is private by default (loop iterator).
    // collapse(2) is optional but good for performance.
    
    #pragma omp parallel for private(j, k)
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            int sum = 0; // Declaring inside makes it automatically private
            for (k = 0; k < N; k++) {
                // A[i][k] * B[k][j] (dummy logic)
                sum += 1; 
            }
            C[i][j] = sum;
        }
    }
    printf("Matrix mult complete (simulated)\n");
}

// =================================================================
// EXAM QUESTION 3: TASKS & RECURSION (Lab 03)
// Question: "Implement a parallel Fibonacci or Tree Traversal"
// =================================================================
int fib(int n) {
    if (n < 2) return n;
    int x, y;

    // If work is small, do it sequentially (optimization)
    if (n < 20) return fib(n-1) + fib(n-2);

    #pragma omp task shared(x)
    x = fib(n - 1);

    #pragma omp task shared(y)
    y = fib(n - 2);

    #pragma omp taskwait // MUST WAIT for children
    return x + y;
}

void task_exam() {
    printf("\n--- Exam Q3: Task Parallelism ---\n");
    int res;
    #pragma omp parallel
    {
        #pragma omp single
        {
            res = fib(10); // Small number for demo
        }
    }
    printf("Fib(10) Result: %d\n", res);
}

// =================================================================
// MAIN
// =================================================================
int main() {
    // Uncomment the function you want to test
    basic_threading();
    loop_scheduling();
    synchronization_demo();
    manual_reduction_exam();
    matrix_mult_exam();
    task_exam();
    return 0;
}

/* ================================================================================
  RISC-V ASSEMBLY CHEAT SHEET (Labs 07, 08, 09)
================================================================================

1. [cite_start]DATA TYPES & REGISTERS [cite: 165]
   x0 (zero): Always 0.
   x1 (ra):   Return Address.
   x2 (sp):   Stack Pointer.
   x5-x7, x28-x31 (t0-t6): Temporaries (Not saved across calls).
   x8-x9, x18-x27 (s0-s11): Saved registers (Must be preserved).
   x10-x17 (a0-a7): Arguments and Return values.

2. [cite_start]ARITHMETIC INSTRUCTIONS [cite: 71, 77]
   add  rd, rs1, rs2      // rd = rs1 + rs2
   sub  rd, rs1, rs2      // rd = rs1 - rs2
   addi rd, rs1, imm      // rd = rs1 + 10 (Immediate)
   mul  rd, rs1, rs2      // rd = rs1 * rs2
   
   [cite_start]// DIVISION (Special Note from Lab 08) [cite: 101, 106]
   // Your lab uses the MIPS-style Hi/Lo registers for div.
   div  rs1, rs2          // Divide rs1 by rs2. Quotient -> lo register.
   mflo rd                // Move quotient from lo to rd.

3. [cite_start]DATA TRANSFER (Memory) [cite: 65, 120]
   // Loading Constants
   lui  rd, 0x12345       // Load upper 20 bits
   addi rd, rd, 0x678     // Add lower 12 bits

   // Memory Access
   lw   rd, offset(rs1)   // Load Word (4 bytes) from address rs1 + offset
   sw   rs2, offset(rs1)  // Store Word (4 bytes) from rs2 to address rs1 + offset
   // NOTE: Offset is in BYTES. Array index i -> offset = i * 4.

4. [cite_start]LOGICAL & SHIFTS [cite: 130, 514]
   and  rd, rs1, rs2      // Bitwise AND
   or   rd, rs1, rs2      // Bitwise OR
   xor  rd, rs1, rs2      // Bitwise XOR
   
   slli rd, rs1, imm      // Shift Left Logical (Multiply by 2^imm)
   srli rd, rs1, imm      // Shift Right Logical (Divide by 2^imm, zero fill)
   srai rd, rs1, imm      // Shift Right Arithmetic (Divide signed, sign extend)

5. [cite_start]CONTROL FLOW (If/Else/Loops) [cite: 537, 578]
   
   // Branching
   beq  rs1, rs2, label   // Branch if Equal
   bne  rs1, rs2, label   // Branch if Not Equal
   blt  rs1, rs2, label   // Branch if Less Than
   bge  rs1, rs2, label   // Branch if Greater or Equal
   j    label             // Unconditional Jump

   // EXAMPLE: IF (x10 == x11) { x5 = 10 } ELSE { x5 = 20 }
       bne  x10, x11, else_block
       li   x5, 10
       j    end_block
   else_block:
       li   x5, 20
   end_block:

   // EXAMPLE: FOR LOOP (i = 0; i < 10; i++) (i is x5, limit x6=10)
       li   x5, 0         // i = 0
       li   x6, 10        // limit = 10
   loop_start:
       bge  x5, x6, loop_end  // Exit if i >= 10
       
       // ... Body of loop ...
       
       addi x5, x5, 1     // i++
       j    loop_start
   loop_end:

   // EXAMPLE: WHILE LOOP (while x5 > 0)
   loop_start:
       beq  x5, x0, loop_end  // Exit if x5 == 0
       addi x5, x5, -1        // x5--
       j    loop_start
   loop_end:
*/
