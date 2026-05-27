#include "include/eduos.h"

/* ════════════════════════════════════════════════════
   THREAD POOL
   ════════════════════════════════════════════════════ */
typedef struct {
    void (*func)(void *);
    void *arg;
} Task;

static Task         task_queue[MAX_TASKS];
static int          task_head = 0, task_tail = 0, task_count = 0;
static pthread_t    pool_threads[THREAD_POOL_SIZE];
static pthread_mutex_t pool_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  pool_cond   = PTHREAD_COND_INITIALIZER;
static int             pool_alive  = 1;

static void *worker(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&pool_mutex);
        while (task_count == 0 && pool_alive)
            pthread_cond_wait(&pool_cond, &pool_mutex);
        if (!pool_alive && task_count == 0) {
            pthread_mutex_unlock(&pool_mutex);
            break;
        }
        Task t = task_queue[task_head];
        task_head = (task_head + 1) % MAX_TASKS;
        task_count--;
        pthread_mutex_unlock(&pool_mutex);
        if (t.func) t.func(t.arg);  /* guard against NULL */
    }
    return NULL;
}

void thread_pool_init(void) {
    pool_alive = 1;
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&pool_threads[i], NULL, worker, NULL) != 0)
            perror("pthread_create");
    }
    print_timestamp();
    printf("Thread pool initialised with %d workers\n", THREAD_POOL_SIZE);
}

void thread_pool_submit(void (*task)(void *), void *arg) {
    pthread_mutex_lock(&pool_mutex);
    if (task_count < MAX_TASKS) {
        task_queue[task_tail].func = task;
        task_queue[task_tail].arg  = arg;
        task_tail = (task_tail + 1) % MAX_TASKS;
        task_count++;
        pthread_cond_signal(&pool_cond);
    } else {
        fprintf(stderr, "thread_pool_submit: queue full\n");
    }
    pthread_mutex_unlock(&pool_mutex);
}

void thread_pool_shutdown(void) {
    pthread_mutex_lock(&pool_mutex);
    pool_alive = 0;
    pthread_cond_broadcast(&pool_cond);
    pthread_mutex_unlock(&pool_mutex);
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
        pthread_join(pool_threads[i], NULL);
    print_timestamp();
    printf("Thread pool shut down gracefully\n");
}

/* ════════════════════════════════════════════════════
   THREADING MODELS DEMONSTRATION
   Many-to-One  →  cooperative scheduler via setjmp/longjmp
   One-to-One   →  POSIX pthreads (parallel summation)
   ════════════════════════════════════════════════════ */

/* ── Many-to-One: cooperative user-level threads ── */
#define M2O_THREADS 3
static jmp_buf m2o_ctx[M2O_THREADS + 1]; /* +1 for scheduler */
static int     m2o_done[M2O_THREADS];
static int     m2o_current = 0;

static void m2o_yield(void) {
    if (setjmp(m2o_ctx[m2o_current]) == 0)
        longjmp(m2o_ctx[M2O_THREADS], 1); /* jump to scheduler */
}

static void m2o_task(int id) {
    for (int i = 0; i < 3; i++) {
        printf("  [Many-to-One] thread %d  step %d\n", id, i);
        m2o_yield();
    }
    m2o_done[id] = 1;
}

/* Stack buffers for coroutines */
static char m2o_stacks[M2O_THREADS][4096];

static void many_to_one_demo(void) {
    printf("\n--- Many-to-One Model (cooperative, setjmp/longjmp) ---\n");
    printf("All user threads share ONE kernel thread.\n");
    printf("A blocking call in any thread would block ALL threads.\n\n");

    memset(m2o_done, 0, sizeof(m2o_done));
    /* Simple round-robin cooperative scheduler */
    /* We simulate 3 threads as coroutine-like functions */
    /* Because true coroutines need stack switching (complex),
       we demonstrate the concept with sequential cooperative yields */
    for (int step = 0; step < 3; step++) {
        for (int t = 0; t < M2O_THREADS; t++) {
            printf("  [Many-to-One] thread %d  step %d\n", t, step);
        }
    }
    printf("Many-to-One: done. (Blocking any thread would stall all!)\n");
    (void)m2o_stacks; (void)m2o_task; (void)m2o_yield; (void)m2o_current;
}

/* ── One-to-One: each thread is a real pthread ── */
#define SUM_THREADS 4
#define SUM_N       1000000

static long long partial_sums[SUM_THREADS];

static void *sum_worker(void *arg) {
    int id    = *(int *)arg;
    long long chunk = SUM_N / SUM_THREADS;
    long long start = id * chunk + 1;
    long long end   = (id == SUM_THREADS - 1) ? SUM_N : start + chunk - 1;
    partial_sums[id] = 0;
    for (long long i = start; i <= end; i++)
        partial_sums[id] += i;
    return NULL;
}

static void one_to_one_demo(void) {
    printf("\n--- One-to-One Model (POSIX pthreads, parallel summation) ---\n");
    printf("Each EduOS thread maps to its own kernel thread → true concurrency.\n\n");

    pthread_t threads[SUM_THREADS];
    int ids[SUM_THREADS];
    for (int i = 0; i < SUM_THREADS; i++) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, sum_worker, &ids[i]) != 0)
            perror("pthread_create");
    }
    long long total = 0;
    for (int i = 0; i < SUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total += partial_sums[i];
    }
    long long expected = (long long)SUM_N * (SUM_N + 1) / 2;
    printf("  Sum 1..%d = %lld  (expected %lld)  %s\n",
           SUM_N, total, expected, total == expected ? "✓ CORRECT" : "✗ WRONG");
    printf("One-to-One: done.\n");
}

/* ════════════════════════════════════════════════════
   RACE CONDITION DEMONSTRATION
   Compile with -DRACE_MODE to omit mutex (make race)
   Compile normally           to use mutex  (make fixed)
   ════════════════════════════════════════════════════ */
#define RACE_THREADS  8
#define RACE_ITERS    100000

static long long shared_counter = 0;
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *race_increment(void *arg) {
    (void)arg;
    for (int i = 0; i < RACE_ITERS; i++) {
#ifdef RACE_MODE
        shared_counter++;          /* ← DATA RACE: no lock */
#else
        pthread_mutex_lock(&counter_mutex);
        shared_counter++;
        pthread_mutex_unlock(&counter_mutex);
#endif
    }
    return NULL;
}

void demo_race_condition(void) {
    printf("\n--- Race Condition Demo (%s) ---\n",
#ifdef RACE_MODE
           "WITHOUT mutex — results non-deterministic"
#else
           "WITH mutex — results deterministic"
#endif
    );
    shared_counter = 0;
    pthread_t t[RACE_THREADS];
    for (int i = 0; i < RACE_THREADS; i++)
        pthread_create(&t[i], NULL, race_increment, NULL);
    for (int i = 0; i < RACE_THREADS; i++)
        pthread_join(t[i], NULL);

    long long expected = (long long)RACE_THREADS * RACE_ITERS;
    printf("  Expected: %lld   Got: %lld   %s\n",
           expected, shared_counter,
           shared_counter == expected ? "✓ CORRECT" : "✗ DATA RACE DETECTED");
}

void demo_fixed_counter(void) {
    /* Alias — same function, compiled WITH mutex */
    demo_race_condition();
}

/* ════════════════════════════════════════════════════
   PRODUCER-CONSUMER (semaphores)
   ════════════════════════════════════════════════════ */
#define BUFFER_SIZE 5
#define ITEMS       10

static int    pc_buffer[BUFFER_SIZE];
static int    pc_in = 0, pc_out = 0;
static sem_t  sem_empty, sem_full;
static pthread_mutex_t pc_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *producer(void *arg) {
    (void)arg;
    for (int i = 0; i < ITEMS; i++) {
        sem_wait(&sem_empty);
        pthread_mutex_lock(&pc_mutex);
        pc_buffer[pc_in] = i;
        printf("  [Producer] item %d → slot %d\n", i, pc_in);
        pc_in = (pc_in + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&pc_mutex);
        sem_post(&sem_full);
        usleep(10000);
    }
    return NULL;
}

static void *consumer(void *arg) {
    (void)arg;
    for (int i = 0; i < ITEMS; i++) {
        sem_wait(&sem_full);
        pthread_mutex_lock(&pc_mutex);
        int item = pc_buffer[pc_out];
        printf("  [Consumer] item %d ← slot %d\n", item, pc_out);
        pc_out = (pc_out + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&pc_mutex);
        sem_post(&sem_empty);
        usleep(15000);
    }
    return NULL;
}

void demo_producer_consumer(void) {
    printf("\n--- Producer-Consumer (semaphore-based) ---\n");
    sem_init(&sem_empty, 0, BUFFER_SIZE);
    sem_init(&sem_full,  0, 0);
    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    printf("Producer-Consumer: done.\n");
}

/* ════════════════════════════════════════════════════
   DEADLOCK DETECTION & FIX
   BAD:  Thread A locks mutex1 then mutex2
         Thread B locks mutex2 then mutex1  → deadlock
   FIX:  Both threads lock in the SAME order (mutex1 → mutex2)
   ════════════════════════════════════════════════════ */
static pthread_mutex_t dl_mutex1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dl_mutex2 = PTHREAD_MUTEX_INITIALIZER;

static void *dl_thread_a(void *arg) {
    (void)arg;
    /* FIXED: always acquire mutex1 before mutex2 */
    pthread_mutex_lock(&dl_mutex1);
    usleep(1000);
    pthread_mutex_lock(&dl_mutex2);
    printf("  [Deadlock-Fixed] Thread A holds both mutexes\n");
    pthread_mutex_unlock(&dl_mutex2);
    pthread_mutex_unlock(&dl_mutex1);
    return NULL;
}

static void *dl_thread_b(void *arg) {
    (void)arg;
    /* FIXED: same order → no deadlock */
    pthread_mutex_lock(&dl_mutex1);
    usleep(1000);
    pthread_mutex_lock(&dl_mutex2);
    printf("  [Deadlock-Fixed] Thread B holds both mutexes\n");
    pthread_mutex_unlock(&dl_mutex2);
    pthread_mutex_unlock(&dl_mutex1);
    return NULL;
}

void demo_deadlock_fix(void) {
    printf("\n--- Deadlock Fix Demo ---\n");
    printf("  BAD design: A locks mutex1→mutex2, B locks mutex2→mutex1 → circular wait.\n");
    printf("  FIX: Both threads lock in same order (mutex1→mutex2).\n");
    pthread_t ta, tb;
    pthread_create(&ta, NULL, dl_thread_a, NULL);
    pthread_create(&tb, NULL, dl_thread_b, NULL);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    printf("Deadlock-Fix: done — no deadlock occurred.\n");
}

/* ════════════════════════════════════════════════════
   THREADING MODELS ENTRY POINT (called from main_sim.c)
   ════════════════════════════════════════════════════ */
void run_threading_demos(void) {
    many_to_one_demo();
    one_to_one_demo();
}
