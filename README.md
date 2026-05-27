# EduOS — Educational Operating System Simulator
**Module:** CS 2104 – Operating Systems | **Semester:** III  
**Student:** Titus Nyoni | **Reg Number:** [25311351020]  
**GitHub:** https://github.com/TitusNyoni106-svg/EduOs

EduOS is a multi-component simulator that demonstrates real operating-system concepts through working code. It integrates a C core (process management, threading, IPC) with a Python scheduling simulator, all orchestrated by a Python controller. The project covers process lifecycle management, POSIX threading, race conditions, IPC mechanisms, and four CPU scheduling algorithms with visualisation.

---

## Prerequisites

- GCC (gcc ≥ 9) with pthread support
- GNU Make
- Python 3.8+
- pip
- valgrind (optional, for memory checks)

---

## Build & Run Instructions

### C Core

```bash
cd c_core
make all          # build eduos binary (zero warnings)
./eduos           # run the simulator

make race         # build WITHOUT mutex → race condition demo
make fixed        # build WITH mutex    → correct counter demo
make memcheck     # run valgrind memory check
make clean        # remove build artefacts
```

### Python Scheduler

```bash
cd python_scheduler
pip install -r requirements.txt

# Random processes
python scheduler_sim.py --random 10 --seed 42

# From CSV file
python scheduler_sim.py --file sample_processes.csv

# From PCB snapshot (C simulator output)
python scheduler_sim.py --pcb ../c_core/pcb_snapshot.json

# Round Robin with custom quantum
python scheduler_sim.py --random 8 --quantum 3

# Thread mode
python scheduler_sim.py --random 6 --mode thread
```

### Full Integration (Recommended)

```bash
# From project root — runs everything end-to-end
python3 controller/main_controller.py
```

---

## Annotated Directory Tree

```
EduOS/
├── README.md                   ← This file
├── .gitignore                  ← Ignores build artefacts, pycache, etc.
├── simulation_report.json      ← Auto-generated metrics report (after run)
│
├── c_core/                     ← C low-level simulator (Part 2)
│   ├── Makefile                ← Targets: all, clean, race, fixed, memcheck
│   ├── include/
│   │   └── eduos.h             ← Shared PCB struct + all function prototypes
│   ├── process_manager.c       ← PCB table, edu_fork/exec/wait/exit, JSON serialiser
│   ├── thread_manager.c        ← Thread pool, threading models, race demo, IPC helpers
│   ├── ipc_module.c            ← Shared memory + pipe IPC implementations
│   └── main_sim.c              ← Driver that exercises all C components
│
├── python_scheduler/           ← Python scheduling simulator (Part 3)
│   ├── scheduler_sim.py        ← FCFS, SJF, Priority, Round Robin + Gantt charts
│   ├── sample_processes.csv    ← Sample input file (schema: pid,name,arrival,burst,priority)
│   └── requirements.txt        ← matplotlib, tabulate
│
├── controller/                 ← Integration layer (Part 4)
│   └── main_controller.py      ← Launches C binary, reads snapshot, runs scheduler, writes report
│
└── docs/
    ├── report.pdf              ← Written report
    └── screenshots/            ← Auto-generated Gantt + comparison charts (.png)
```

---

## Screenshots

> Screenshots are auto-generated into `docs/screenshots/` when you run the simulator.

**1. C Simulator Running — Process Management**  
Shows `edu_fork`, `edu_exec`, `edu_wait`, `edu_exit` with timestamped logs and the `edu_ps` table.

**2. Python Scheduler — Algorithm Comparison Table**  
Shows all 4 algorithms side-by-side with WT, TAT, RT, CPU Utilisation, Throughput.

**3. Gantt Charts**  
Generated as PNG files in `docs/screenshots/`: `gantt_FCFS.png`, `gantt_SJF.png`, `gantt_Priority.png`, `gantt_RRq2.png`, `gantt_RR_q4.png`, `comparison_all_algorithms.png`.

---

## Valgrind Output

```
Run: valgrind --leak-check=full ./eduos

==XXXXX== HEAP SUMMARY:
==XXXXX==     in use at exit: 0 bytes in 0 blocks
==XXXXX==   total heap usage: X allocs, X frees, X bytes allocated
==XXXXX==
==XXXXX== All heap blocks were freed -- no leaks are possible
==XXXXX==
==XXXXX== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

> Paste your actual valgrind output here after running `make memcheck`.

---

## Race Condition Demo

```bash
make race   # compile WITHOUT mutex

# Sample output (5 runs — non-deterministic):
Expected: 800000   Got: 743291   ✗ DATA RACE DETECTED
Expected: 800000   Got: 761088   ✗ DATA RACE DETECTED
Expected: 800000   Got: 800000   (lucky)
Expected: 800000   Got: 712004   ✗ DATA RACE DETECTED
Expected: 800000   Got: 759932   ✗ DATA RACE DETECTED

make fixed  # compile WITH mutex

Expected: 800000   Got: 800000   ✓ CORRECT
```

The race occurs because 8 threads simultaneously do `shared_counter++` without synchronisation. This is a read-modify-write operation that is NOT atomic. Adding `pthread_mutex_lock/unlock` makes it deterministic.

---

## CSV Schema

```
pid,name,arrival_time,burst_time,priority
1,init,0,6,2
2,bash,1,8,1
```

---

## Challenges Encountered

| Challenge | How I Resolved It |
|---|---|
| `edu_wait` polling loop hung forever | Added a 100-iteration limit with `nanosleep` instead of infinite `usleep` loop |
| gcc `-Wall -Wextra` format-truncation warning in `snprintf` | Used `%.50s` format specifier to limit input length before appending `_child` |
| `usleep` implicit declaration under `-std=c11` | Added `#define _DEFAULT_SOURCE` before includes to enable POSIX extensions |
| Shared memory mutex across processes | Used `pthread_mutexattr_setpshared(PTHREAD_PROCESS_SHARED)` |
| Python scheduler importing matplotlib on headless system | Added `matplotlib.use("Agg")` backend before import |

---

## References

- Abraham Silberschatz, Peter B. Galvin, Greg Gagne — *Operating System Concepts* (10th Ed.)
- Linux `man` pages: `fork(2)`, `execve(2)`, `wait(2)`, `_exit(2)`, `shm_open(3)`, `mmap(2)`, `pipe(2)`, `pthread_create(3)`, `sem_init(3)`
- GNU Make Manual — https://www.gnu.org/software/make/manual/
- Python `subprocess` documentation — https://docs.python.org/3/library/subprocess.html
- Lecture notes: CS 2104 Operating Systems, Semester III

---

## Bonus

- Thread scheduling mode with context-switch overhead (`--mode thread`)
- Ageing implementation in Priority Scheduling (every 3 time units)
- Deadlock detection and fix demonstration with consistent lock ordering
- Producer-Consumer with semaphores (`sem_init`, `sem_wait`, `sem_post`)
