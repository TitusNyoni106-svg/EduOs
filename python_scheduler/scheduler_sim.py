#!/usr/bin/env python3
"""
EduOS Scheduling Simulator — CS 2104 Operating Systems
Implements: FCFS, SJF, Priority (with ageing), Round Robin
"""

import argparse
import csv
import json
import random
import sys
from copy import deepcopy
from typing import List, Dict, Tuple

# ── Try importing visualisation libs (optional) ──────────────────────────────
try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

try:
    from tabulate import tabulate
    HAS_TABULATE = True
except ImportError:
    HAS_TABULATE = False

# ─────────────────────────────────────────────────────────────────────────────
# Data helpers
# ─────────────────────────────────────────────────────────────────────────────

def make_process(pid: int, arrival: int, burst: int, priority: int,
                 name: str = "") -> Dict:
    return {
        "pid":          pid,
        "name":         name or f"P{pid}",
        "arrival_time": arrival,
        "burst_time":   burst,
        "priority":     priority,
        "remaining":    burst,
        # filled by scheduler
        "start_time":   -1,
        "completion":   0,
    }

def compute_metrics(procs: List[Dict]) -> List[Dict]:
    for p in procs:
        p["tat"] = p["completion"] - p["arrival_time"]
        p["wt"]  = p["tat"] - p["burst_time"]
        p["rt"]  = p["start_time"] - p["arrival_time"]
    return procs

def aggregate(procs: List[Dict], total_time: int) -> Dict:
    n = len(procs)
    busy = sum(p["burst_time"] for p in procs)
    return {
        "avg_wt":       sum(p["wt"]  for p in procs) / n,
        "avg_tat":      sum(p["tat"] for p in procs) / n,
        "avg_rt":       sum(p["rt"]  for p in procs) / n,
        "cpu_util":     busy / total_time * 100 if total_time else 0,
        "throughput":   n / total_time if total_time else 0,
    }

# ─────────────────────────────────────────────────────────────────────────────
# Scheduling algorithms
# Each returns: (schedule, procs_with_metrics, total_time)
# schedule = list of (pid, start, end) tuples
# ─────────────────────────────────────────────────────────────────────────────

def fcfs(processes: List[Dict]) -> Tuple[List, List, int]:
    """First Come First Served — non-preemptive."""
    procs   = deepcopy(processes)
    procs.sort(key=lambda p: (p["arrival_time"], p["pid"]))
    schedule = []
    clock    = 0

    for p in procs:
        if clock < p["arrival_time"]:
            clock = p["arrival_time"]
        p["start_time"]  = clock
        start = clock
        clock += p["burst_time"]
        p["completion"]  = clock
        schedule.append((p["pid"], start, clock))

    return schedule, compute_metrics(procs), clock


def sjf(processes: List[Dict]) -> Tuple[List, List, int]:
    """Shortest Job First — non-preemptive."""
    procs    = deepcopy(processes)
    done     = []
    schedule = []
    clock    = 0
    ready    = []
    remaining = list(procs)

    while remaining or ready:
        # admit arrivals
        arrived = [p for p in remaining if p["arrival_time"] <= clock]
        for p in arrived:
            ready.append(p)
            remaining.remove(p)

        if not ready:
            clock = remaining[0]["arrival_time"]
            continue

        # pick shortest burst; tie → lower pid (FCFS order)
        ready.sort(key=lambda p: (p["burst_time"], p["pid"]))
        p = ready.pop(0)
        p["start_time"] = clock
        start = clock
        clock += p["burst_time"]
        p["completion"] = clock
        schedule.append((p["pid"], start, clock))
        done.append(p)

    return schedule, compute_metrics(done), clock


def priority_scheduling(processes: List[Dict]) -> Tuple[List, List, int]:
    """
    Priority Scheduling — non-preemptive.
    Lower priority number = higher urgency.
    Ageing: every 3 time units a waiting process gains +1 priority (number −1).
    """
    procs     = deepcopy(processes)
    done      = []
    schedule  = []
    clock     = 0
    ready     = []
    remaining = list(procs)
    last_age  = 0

    while remaining or ready:
        arrived = [p for p in remaining if p["arrival_time"] <= clock]
        for p in arrived:
            ready.append(p)
            remaining.remove(p)

        if not ready:
            clock = remaining[0]["arrival_time"]
            continue

        # ── Ageing: every 3 ticks, boost waiting processes ──────────────
        if clock - last_age >= 3:
            for p in ready:
                if p["priority"] > 0:
                    p["priority"] -= 1   # lower number = higher priority
            last_age = clock

        # pick highest priority; tie → lower pid
        ready.sort(key=lambda p: (p["priority"], p["pid"]))
        p = ready.pop(0)
        p["start_time"] = clock
        start = clock
        clock += p["burst_time"]
        p["completion"] = clock
        schedule.append((p["pid"], start, clock))
        done.append(p)

    return schedule, compute_metrics(done), clock


def round_robin(processes: List[Dict], quantum: int = 2) -> Tuple[List, List, int]:
    """
    Round Robin — preemptive, user-defined time quantum.
    Maintains a proper FIFO ready queue.
    """
    procs     = deepcopy(processes)
    procs.sort(key=lambda p: (p["arrival_time"], p["pid"]))
    queue     = []
    done      = []
    schedule  = []
    clock     = 0
    remaining = list(procs)

    # seed with processes arriving at time 0
    for p in list(remaining):
        if p["arrival_time"] <= clock:
            queue.append(p)
            remaining.remove(p)

    while queue or remaining:
        if not queue:
            clock = remaining[0]["arrival_time"]
            arrived = [p for p in remaining if p["arrival_time"] <= clock]
            for p in arrived:
                queue.append(p)
                remaining.remove(p)

        p = queue.pop(0)

        if p["start_time"] == -1:
            p["start_time"] = clock

        run = min(quantum, p["remaining"])
        start = clock
        clock += run
        p["remaining"] -= run
        schedule.append((p["pid"], start, clock))

        # admit new arrivals that happened during this quantum
        arrived = [pr for pr in remaining if pr["arrival_time"] <= clock]
        for pr in sorted(arrived, key=lambda x: x["arrival_time"]):
            queue.append(pr)
            remaining.remove(pr)

        if p["remaining"] == 0:
            p["completion"] = clock
            done.append(p)
        else:
            queue.append(p)   # re-enqueue

    return schedule, compute_metrics(done), clock

# ─────────────────────────────────────────────────────────────────────────────
# Input / output
# ─────────────────────────────────────────────────────────────────────────────

def generate_random(n: int, seed: int = None) -> List[Dict]:
    if seed is not None:
        random.seed(seed)
    procs = []
    for i in range(1, n + 1):
        procs.append(make_process(
            pid=i,
            arrival=random.randint(0, n * 2),
            burst=random.randint(1, 12),
            priority=random.randint(0, 5),
        ))
    return procs

def load_csv(path: str) -> List[Dict]:
    procs = []
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            procs.append(make_process(
                pid=int(row["pid"]),
                arrival=int(row["arrival_time"]),
                burst=int(row["burst_time"]),
                priority=int(row.get("priority", 0)),
                name=row.get("name", ""),
            ))
    return procs

def load_json(path: str) -> List[Dict]:
    with open(path) as f:
        data = json.load(f)
    procs = []
    for row in data:
        # Accept both raw process dicts and PCB snapshot format
        pid      = row.get("pid", 0)
        arrival  = row.get("arrival_time", 0)
        burst    = row.get("burst_time", row.get("burst", 1))
        priority = row.get("priority", 0)
        name     = row.get("name", f"P{pid}")
        state    = row.get("state", "")
        # Skip non-runnable PCBs from snapshot
        if state in ("TERMINATED",):
            burst = max(burst, 1)
        procs.append(make_process(pid, arrival, burst, priority, name))
    return procs

def print_results(name: str, procs: List[Dict], agg: Dict):
    print(f"\n{'='*60}")
    print(f"  Algorithm: {name}")
    print(f"{'='*60}")

    headers = ["PID","Name","Arrival","Burst","Completion","TAT","WT","RT"]
    rows = [[p["pid"], p["name"], p["arrival_time"], p["burst_time"],
             p["completion"], p["tat"], p["wt"], p["rt"]] for p in procs]

    if HAS_TABULATE:
        print(tabulate(rows, headers=headers, tablefmt="rounded_outline"))
    else:
        print("  " + "  ".join(f"{h:<10}" for h in headers))
        for r in rows:
            print("  " + "  ".join(f"{str(v):<10}" for v in r))

    print(f"\n  Avg WT: {agg['avg_wt']:.2f}  |  Avg TAT: {agg['avg_tat']:.2f}  "
          f"|  Avg RT: {agg['avg_rt']:.2f}")
    print(f"  CPU Util: {agg['cpu_util']:.1f}%  |  "
          f"Throughput: {agg['throughput']:.4f} proc/unit")

def print_comparison(results: Dict):
    print(f"\n{'='*70}")
    print("  ALGORITHM COMPARISON")
    print(f"{'='*70}")
    headers = ["Algorithm", "Avg WT", "Avg TAT", "Avg RT", "CPU Util%", "Throughput"]
    rows = []
    for name, (_, _, agg) in results.items():
        rows.append([name,
                     f"{agg['avg_wt']:.2f}",
                     f"{agg['avg_tat']:.2f}",
                     f"{agg['avg_rt']:.2f}",
                     f"{agg['cpu_util']:.1f}",
                     f"{agg['throughput']:.4f}"])
    if HAS_TABULATE:
        print(tabulate(rows, headers=headers, tablefmt="double_grid"))
    else:
        print("  " + "  ".join(f"{h:<14}" for h in headers))
        for r in rows:
            print("  " + "  ".join(f"{str(v):<14}" for v in r))

# ─────────────────────────────────────────────────────────────────────────────
# Visualisation
# ─────────────────────────────────────────────────────────────────────────────

COLORS = ["#e74c3c","#3498db","#2ecc71","#f39c12",
          "#9b59b6","#1abc9c","#e67e22","#34495e"]

def gantt_chart(name: str, schedule: List[Tuple], procs: List[Dict],
                out_path: str):
    if not HAS_MATPLOTLIB:
        print(f"  [skip] matplotlib not installed — cannot draw Gantt for {name}")
        return

    pid_list  = sorted({s[0] for s in schedule})
    color_map = {pid: COLORS[i % len(COLORS)] for i, pid in enumerate(pid_list)}
    name_map  = {p["pid"]: p["name"] for p in procs}

    fig, ax = plt.subplots(figsize=(max(12, schedule[-1][2] // 2), 4))
    yticks, ylabels = [], []

    for row_i, pid in enumerate(pid_list):
        segments = [(s, e) for p, s, e in schedule if p == pid]
        for (s, e) in segments:
            ax.barh(row_i, e - s, left=s, height=0.5,
                    color=color_map[pid], edgecolor="white")
        yticks.append(row_i)
        ylabels.append(f"{name_map[pid]} (PID {pid})")

    # idle gaps
    max_time = schedule[-1][2]
    covered  = set()
    for _, s, e in schedule:
        covered.update(range(s, e))
    idle_start = None
    for t in range(max_time + 1):
        if t not in covered and idle_start is None:
            idle_start = t
        elif t in covered and idle_start is not None:
            ax.barh(len(pid_list), t - idle_start, left=idle_start,
                    height=0.3, color="lightgrey", edgecolor="white")
            idle_start = None

    ax.set_yticks(yticks)
    ax.set_yticklabels(ylabels)
    ax.set_xlabel("Time")
    ax.set_title(f"Gantt Chart — {name}")
    ax.set_xticks(range(0, max_time + 1))
    ax.grid(axis="x", linestyle="--", alpha=0.4)

    patches = [mpatches.Patch(color=color_map[p], label=name_map[p])
               for p in pid_list]
    patches.append(mpatches.Patch(color="lightgrey", label="Idle"))
    ax.legend(handles=patches, loc="upper right", fontsize=7)

    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    plt.close()
    print(f"  Gantt chart saved → {out_path}")


def comparison_charts(results: Dict, out_path: str):
    if not HAS_MATPLOTLIB:
        return

    names   = list(results.keys())
    avg_wt  = [results[n][2]["avg_wt"]   for n in names]
    avg_tat = [results[n][2]["avg_tat"]  for n in names]
    cpu     = [results[n][2]["cpu_util"] for n in names]

    x = range(len(names))
    fig, axes = plt.subplots(1, 3, figsize=(14, 4))

    for ax, values, title, color in zip(
            axes,
            [avg_wt, avg_tat, cpu],
            ["Average Waiting Time", "Average Turnaround Time", "CPU Utilisation (%)"],
            ["#e74c3c", "#3498db", "#2ecc71"]):
        ax.bar(x, values, color=color, edgecolor="black", alpha=0.8)
        ax.set_xticks(x)
        ax.set_xticklabels(names, rotation=15, fontsize=9)
        ax.set_title(title)
        ax.set_ylabel(title)
        for i, v in enumerate(values):
            ax.text(i, v + 0.1, f"{v:.2f}", ha="center", fontsize=8)

    fig.suptitle("Scheduling Algorithm Comparison", fontsize=13, fontweight="bold")
    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    plt.close()
    print(f"  Comparison chart saved → {out_path}")

# ─────────────────────────────────────────────────────────────────────────────
# Thread scheduling mode
# ─────────────────────────────────────────────────────────────────────────────

def thread_mode(processes: List[Dict], quantum: int):
    """
    Thread scheduling mode.
    Processes with the same PID prefix are treated as threads in the same group.
    Context-switch overhead (1 unit) is added between different PID groups.
    """
    print("\n" + "="*60)
    print("  THREAD SCHEDULING MODE (Round Robin with affinity)")
    print("="*60)
    print("  Each 'process' = a thread.  Threads from the same PID group")
    print("  are shown together. Context-switch cost = 1 unit added on")
    print("  every switch between different PID groups.\n")

    procs     = deepcopy(processes)
    procs.sort(key=lambda p: (p["arrival_time"], p["pid"]))
    queue     = []
    done      = []
    schedule  = []
    clock     = 0
    remaining = list(procs)
    last_pid  = None
    ctx_switches = 0

    for p in list(remaining):
        if p["arrival_time"] <= clock:
            queue.append(p)
            remaining.remove(p)

    while queue or remaining:
        if not queue:
            clock = remaining[0]["arrival_time"]
            arrived = [p for p in remaining if p["arrival_time"] <= clock]
            for p in arrived:
                queue.append(p)
                remaining.remove(p)

        p = queue.pop(0)

        # context-switch penalty between different PID groups
        if last_pid is not None and p["pid"] != last_pid:
            clock += 1   # 1 unit context-switch overhead
            ctx_switches += 1

        if p["start_time"] == -1:
            p["start_time"] = clock

        run = min(quantum, p["remaining"])
        start = clock
        clock += run
        p["remaining"] -= run
        last_pid = p["pid"]

        schedule.append((p["pid"], start, clock))

        arrived = [pr for pr in remaining if pr["arrival_time"] <= clock]
        for pr in sorted(arrived, key=lambda x: x["arrival_time"]):
            queue.append(pr)
            remaining.remove(pr)

        if p["remaining"] == 0:
            p["completion"] = clock
            done.append(p)
        else:
            queue.append(p)

    compute_metrics(done)
    print(f"  Context switches due to thread-group changes: {ctx_switches}")
    print(f"  Schedule: {[(pid, s, e) for pid, s, e in schedule]}\n")
    agg = aggregate(done, clock)
    print_results("Round Robin (Thread Mode)", done, agg)

# ─────────────────────────────────────────────────────────────────────────────
# CLI
# ─────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="EduOS Scheduling Simulator")
    src = parser.add_mutually_exclusive_group()
    src.add_argument("--random", type=int, metavar="N",
                     help="Generate N random processes")
    src.add_argument("--file",   type=str, metavar="PATH",
                     help="Load processes from CSV or JSON file")
    src.add_argument("--pcb",    type=str, metavar="PATH",
                     help="Load PCB snapshot JSON from C simulator")

    parser.add_argument("--seed",    type=int,  default=42,
                        help="Random seed (default 42)")
    parser.add_argument("--quantum", type=int,  default=2,
                        help="Round Robin time quantum (default 2)")
    parser.add_argument("--mode",    type=str,  default="process",
                        choices=["process","thread"],
                        help="Scheduling mode")
    parser.add_argument("--outdir",  type=str,  default="docs/screenshots",
                        help="Directory to save charts")

    args = parser.parse_args()

    # ── Load processes ────────────────────────────────────────────────────────
    if args.random:
        processes = generate_random(args.random, args.seed)
        print(f"Generated {args.random} random processes (seed={args.seed})")
    elif args.file:
        path = args.file
        if path.endswith(".csv"):
            processes = load_csv(path)
        else:
            processes = load_json(path)
        print(f"Loaded {len(processes)} processes from {path}")
    elif args.pcb:
        processes = load_json(args.pcb)
        print(f"Loaded {len(processes)} PCBs from snapshot {args.pcb}")
    else:
        # Default demo
        processes = [
            make_process(1, 0, 6, 2),
            make_process(2, 1, 8, 1),
            make_process(3, 2, 4, 3),
            make_process(4, 3, 2, 0),
            make_process(5, 4, 5, 2),
        ]
        print("Using built-in demo processes (5 processes)")

    if args.mode == "thread":
        thread_mode(processes, args.quantum)
        return

    import os
    os.makedirs(args.outdir, exist_ok=True)

    # ── Run all 4 algorithms ──────────────────────────────────────────────────
    algos = {
        "FCFS":     fcfs(processes),
        "SJF":      sjf(processes),
        "Priority": priority_scheduling(processes),
        f"RR(q={args.quantum})": round_robin(processes, args.quantum),
    }

    all_results = {}
    for name, (schedule, procs, total_time) in algos.items():
        agg = aggregate(procs, total_time)
        print_results(name, procs, agg)
        all_results[name] = (schedule, procs, agg)

        gantt_chart(name,
                    schedule,
                    procs,
                    f"{args.outdir}/gantt_{name.replace('(','').replace(')','').replace('=','')}.png")

    print_comparison(all_results)

    comparison_charts(all_results,
                      f"{args.outdir}/comparison_all_algorithms.png")

    # ── Also run RR with a second quantum for report ──────────────────────────
    q2 = 4
    name2 = f"RR(q={q2})"
    sch2, procs2, tot2 = round_robin(processes, q2)
    agg2 = aggregate(procs2, tot2)
    print_results(name2, procs2, agg2)
    gantt_chart(name2, sch2, procs2,
                f"{args.outdir}/gantt_RR_q{q2}.png")

    print(f"\n✓ All done. Charts in {args.outdir}/\n")


if __name__ == "__main__":
    main()
