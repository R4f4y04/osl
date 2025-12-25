#define _XOPEN_SOURCE 600 

#include <stdio.h>      
#include <stdlib.h>     
#include <unistd.h>     
#include <sys/wait.h>   
#include <pthread.h>

// MODULE 1: GLOBALS FOR SYNCHRONIZATION

// Mutex (Lab 10)
pthread_mutex_t lock; 
int shared_counter = 0;

// Condition Variable (Lab 11)
pthread_mutex_t cond_lock;
pthread_cond_t cond_var;
int ready_flag = 0; // The data state we check

// Barrier (Lab 11)
pthread_barrier_t my_barrier;

// MODULE 2: PROCESS MANAGEMENT (Labs 7 & 8)
void demo_processes() {
    printf("\n=== [1] Process Management Demo ===\n");
    
    pid_t pid = fork(); // Clone the process

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } 
    else if (pid == 0) {
        // --- CHILD PROCESS ---
        printf("Child: My PID is %d, Parent is %d\n", getpid(), getppid());
        
        // Lab 8: EXEC Family (Replaces this child process with 'ls')
        // NOTE: Code after exec NEVER runs unless exec fails.
        // We use execlp here: (File, Arg0, Arg1, ..., NULL)
        printf("Child: Executing 'ls -l' now...\n");
        execlp("ls", "ls", "-l", NULL);
        
        // If we get here, exec failed
        perror("Exec failed");
        exit(1); 
    } 
    else {
        // --- PARENT PROCESS ---
        printf("Parent: Waiting for child (%d)...\n", pid);
        
        // Lab 7: Wait (Prevents Zombie)
        wait(NULL); 
        printf("Parent: Child finished. No zombies here.\n");
    }
}


// MODULE 3: THREADS & MUTEXES (Labs 9 & 10)
void* thread_mutex_worker(void* arg) {
    // Lab 10: Mutex Locking
    pthread_mutex_lock(&lock);
    
    // CRITICAL SECTION (Only one thread at a time)
    int id = *((int*)arg);
    shared_counter++;
    printf("Thread %d: Incremented counter to %d\n", id, shared_counter);
    
    // Lab 10: Mutex Unlocking (NEVER FORGET THIS)
    pthread_mutex_unlock(&lock);
    
    // Lab 9: Returning value (Must be malloc'd if it's a pointer)
    return NULL;
}

void demo_threads_mutex() {
    printf("\n=== [2] Threads & Mutex Demo ===\n");
    
    // Init Mutex
    pthread_mutex_init(&lock, NULL);
    
    pthread_t t1, t2;
    int id1 = 1, id2 = 2; // Arguments passed by reference

    // Lab 9: Create Threads
    // Syntax: &thread_id, attributes, function, argument_ptr
    pthread_create(&t1, NULL, thread_mutex_worker, &id1);
    pthread_create(&t2, NULL, thread_mutex_worker, &id2);

    // Lab 9: Join Threads (Wait for them)
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    // Lab 10: Destroy Mutex
    pthread_mutex_destroy(&lock);
}

// MODULE 4: CONDITION VARIABLES (Lab 11)

void* signal_worker(void* arg) {
    sleep(1); // Simulate work
    
    pthread_mutex_lock(&cond_lock);
    
    printf("Signal Thread: Setting flag to TRUE and waking everyone up...\n");
    ready_flag = 1; // Change state
    
    // Signal: Wakes up sleeping thread
    // TIP: Always hold lock while signaling (Good Practice)
    pthread_cond_signal(&cond_var); 
    
    pthread_mutex_unlock(&cond_lock);
    return NULL;
}

void* wait_worker(void* arg) {
    pthread_mutex_lock(&cond_lock);
    
    // CRITICAL: Always use WHILE, never IF (Spurious Wakeups)
    while (ready_flag == 0) {
        printf("Wait Thread: Flag is 0. Sleeping...\n");
        
        // Atomically unlocks mutex & sleeps. Re-locks on wakeup.
        pthread_cond_wait(&cond_var, &cond_lock); 
    }
    
    printf("Wait Thread: Awake! Flag is %d. Proceeding.\n", ready_flag);
    pthread_mutex_unlock(&cond_lock);
    return NULL;
}

void demo_condition_vars() {
    printf("\n=== [3] Condition Variable Demo ===\n");
    
    // Init
    pthread_mutex_init(&cond_lock, NULL);
    pthread_cond_init(&cond_var, NULL);
    
    pthread_t t_wait, t_sig;
    pthread_create(&t_wait, NULL, wait_worker, NULL);
    pthread_create(&t_sig, NULL, signal_worker, NULL);
    
    pthread_join(t_wait, NULL);
    pthread_join(t_sig, NULL);
    
    // Destroy
    pthread_mutex_destroy(&cond_lock);
    pthread_cond_destroy(&cond_var);
}

// MODULE 5: BARRIERS (Lab 11)
void* barrier_worker(void* arg) {
    int id = *((int*)arg);
    int sleep_time = (id == 1) ? 1 : 3;
    
    printf("Barrier Thread %d: Working (%ds)...\n", id, sleep_time);
    sleep(sleep_time);
    
    printf("Barrier Thread %d: Waiting at barrier...\n", id);
    
    // STOP HERE until ALL threads arrive
    // P.S. Make sure you pass the ADDRESS (&barrier)
    pthread_barrier_wait(&my_barrier);
    
    printf("Barrier Thread %d: Barrier passed! We leave together.\n", id);
    return NULL;
}

void demo_barriers() {
    printf("\n=== [4] Barrier Demo ===\n");
    
    // Init Barrier for 2 threads
    pthread_barrier_init(&my_barrier, NULL, 2);
    
    pthread_t t1, t2;
    int id1 = 1, id2 = 2;
    
    pthread_create(&t1, NULL, barrier_worker, &id1);
    pthread_create(&t2, NULL, barrier_worker, &id2);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    // Destroy
    pthread_barrier_destroy(&my_barrier);
}

// MAIN DRIVER

int main() {
    // Run demos sequentially
    demo_processes();
    demo_threads_mutex();
    demo_condition_vars();
    demo_barriers();
    
    printf("\nAll systems go. Good luck on the exam!\n");
    return 0;
}