#include "include/eduos.h"

/* ════════════════════════════════════════════════════
   SHARED MEMORY IPC
   Uses shm_open + mmap.  Access-control check via owner_id.
   ════════════════════════════════════════════════════ */
#define SHM_NAME  "/eduos_shm"

typedef struct {
    pthread_mutex_t lock;
    int             owner_id;        /* access control field */
    int             cpu_usage;
    int             mem_usage_kb;
    long long       total_runtime;
    char            last_proc[64];
} SharedMetrics;

void ipc_shared_memory_demo(void) {
    printf("\n--- IPC: POSIX Shared Memory ---\n");

    /* Create shared memory object */
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) { perror("shm_open"); return; }

    if (ftruncate(fd, sizeof(SharedMetrics)) < 0) {
        perror("ftruncate"); close(fd); return;
    }

    SharedMetrics *shm = mmap(NULL, sizeof(SharedMetrics),
                               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); close(fd); return; }
    close(fd);

    /* Initialise */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    shm->owner_id = 42;

    pid_t child = fork();
    if (child < 0) { perror("fork"); return; }

    if (child == 0) {
        /* ── Child process: WRITER ── */
        int requester_id = 42; /* matches owner_id → access granted */

        pthread_mutex_lock(&shm->lock);

        /* Access-control check (Protection ring concept) */
        if (requester_id != shm->owner_id) {
            printf("  [SHM Child] ACCESS DENIED — owner_id mismatch\n");
            pthread_mutex_unlock(&shm->lock);
            _exit(1);
        }

        shm->cpu_usage     = 73;
        shm->mem_usage_kb  = 2048;
        shm->total_runtime = 123456LL;
        strncpy(shm->last_proc, "init_proc", sizeof(shm->last_proc) - 1);

        printf("  [SHM Child]  wrote → cpu=%d%%  mem=%dKB  runtime=%lld  proc=%s\n",
               shm->cpu_usage, shm->mem_usage_kb,
               shm->total_runtime, shm->last_proc);
        pthread_mutex_unlock(&shm->lock);
        _exit(0);
    }

    /* ── Parent process: READER ── */
    int status;
    waitpid(child, &status, 0);

    pthread_mutex_lock(&shm->lock);
    printf("  [SHM Parent] read  → cpu=%d%%  mem=%dKB  runtime=%lld  proc=%s\n",
           shm->cpu_usage, shm->mem_usage_kb,
           shm->total_runtime, shm->last_proc);
    pthread_mutex_unlock(&shm->lock);

    pthread_mutex_destroy(&shm->lock);
    munmap(shm, sizeof(SharedMetrics));
    shm_unlink(SHM_NAME);

    printf("Shared Memory IPC: done.\n");
}

/* ════════════════════════════════════════════════════
   ANONYMOUS PIPE IPC
   Parent serialises a PCB → pipe → child parses & prints
   ════════════════════════════════════════════════════ */
void ipc_pipe_demo(void) {
    printf("\n--- IPC: Anonymous Pipe ---\n");

    /* Build a sample PCB to serialise */
    PCB sample = {
        .pid           = 99,
        .parent_pid    = 1,
        .name          = "pipe_demo_proc",
        .state         = STATE_RUNNING,
        .priority      = 2,
        .burst_time    = 8,
        .arrival_time  = 0,
        .remaining_time= 5,
        .memory_req_kb = 512,
        .thread_count  = 2,
        .exit_code     = 0,
        .owner_id      = 42,
    };
    sample.creation_time = time(NULL);

    int pipefd[2];
    if (pipe(pipefd) < 0) { perror("pipe"); return; }

    pid_t child = fork();
    if (child < 0) { perror("fork"); return; }

    if (child == 0) {
        /* ── Child: READER ── */
        close(pipefd[1]);
        PCB received;
        ssize_t n = read(pipefd[0], &received, sizeof(PCB));
        close(pipefd[0]);

        if (n != sizeof(PCB)) {
            fprintf(stderr, "  [Pipe Child] short read\n");
            _exit(1);
        }

        printf("  [Pipe Child] received PCB → pid=%d  name=%s  state=%s  burst=%d\n",
               received.pid, received.name,
               state_name(received.state), received.burst_time);
        _exit(0);
    }

    /* ── Parent: WRITER ── */
    close(pipefd[0]);
    ssize_t w = write(pipefd[1], &sample, sizeof(PCB));
    close(pipefd[1]);

    if (w != sizeof(PCB))
        perror("write pipe");
    else
        printf("  [Pipe Parent] sent PCB   → pid=%d  name=%s\n",
               sample.pid, sample.name);

    int status;
    waitpid(child, &status, 0);
    printf("Pipe IPC: done.\n");
}
