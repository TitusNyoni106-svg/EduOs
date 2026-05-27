#!/usr/bin/env python3
"""
EduOS Main Controller — CS 2104 Operating Systems
Orchestrates: C binary → PCB snapshot → Python scheduler → JSON report
"""

import json
import os
import subprocess
import sys
import time
import datetime

# ── Paths ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR     = os.path.dirname(SCRIPT_DIR)
C_BINARY     = os.path.join(ROOT_DIR, "c_core", "eduos")
SCHEDULER    = os.path.join(ROOT_DIR, "python_scheduler", "scheduler_sim.py")
PCB_SNAPSHOT = os.path.join(ROOT_DIR, "c_core", "pcb_snapshot.json")
REPORT_PATH  = os.path.join(ROOT_DIR, "simulation_report.json")
SCREENSHOTS  = os.path.join(ROOT_DIR, "docs", "screenshots")


def banner(msg: str):
    print(f"\n{'═'*60}")
    print(f"  {msg}")
    print(f"{'═'*60}")


# ─────────────────────────────────────────────────────────────────────────────
# Step 1 — Build C binary if needed
# ─────────────────────────────────────────────────────────────────────────────
def build_c_binary():
    banner("Step 1: Building C Simulator")
    c_dir = os.path.join(ROOT_DIR, "c_core")
    result = subprocess.run(
        ["make", "all"],
        cwd=c_dir,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print(f"  [ERROR] make failed:\n{result.stderr}")
        sys.exit(1)
    print(f"  ✓ C binary built at: {C_BINARY}")


# ─────────────────────────────────────────────────────────────────────────────
# Step 2 — Launch C binary and capture output in real-time
# ─────────────────────────────────────────────────────────────────────────────
def run_c_simulator():
    banner("Step 2: Running C Simulator (real-time stdout capture)")

    if not os.path.isfile(C_BINARY):
        print(f"  [WARN] C binary not found at {C_BINARY}, building...")
        build_c_binary()

    proc = subprocess.Popen(
        [C_BINARY],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        cwd=os.path.join(ROOT_DIR, "c_core"),
    )

    c_output = []
    print()
    for line in proc.stdout:
        sys.stdout.write(f"  [C] {line}")
        sys.stdout.flush()
        c_output.append(line.rstrip())

    proc.wait()
    if proc.returncode != 0:
        print(f"\n  [ERROR] C binary exited with code {proc.returncode}")
        sys.exit(1)

    print(f"\n  ✓ C simulator done (exit code {proc.returncode})")
    return c_output


# ─────────────────────────────────────────────────────────────────────────────
# Step 3 — Wait for PCB snapshot and detect all-TERMINATED
# ─────────────────────────────────────────────────────────────────────────────
def load_pcb_snapshot():
    banner("Step 3: Loading PCB Snapshot")

    if not os.path.isfile(PCB_SNAPSHOT):
        print(f"  [WARN] {PCB_SNAPSHOT} not found, using demo processes")
        return None

    with open(PCB_SNAPSHOT) as f:
        pcbs = json.load(f)

    terminated = sum(1 for p in pcbs if p.get("state") == "TERMINATED")
    print(f"  Loaded {len(pcbs)} PCBs — {terminated} TERMINATED")

    # Check if all non-init processes are done
    runnable = [p for p in pcbs if p["pid"] != 0]
    all_done = all(p.get("state") == "TERMINATED" for p in runnable)
    print(f"  All runnable processes terminated: {all_done}")
    return pcbs


# ─────────────────────────────────────────────────────────────────────────────
# Step 4 — Hand PCB snapshot to Python scheduler, run all 4 algorithms
# ─────────────────────────────────────────────────────────────────────────────
def run_scheduler(pcbs):
    banner("Step 4: Running Python Scheduler on PCB Snapshot")
    os.makedirs(SCREENSHOTS, exist_ok=True)

    if pcbs:
        src = ["--pcb", PCB_SNAPSHOT]
        print(f"  Using PCB snapshot: {PCB_SNAPSHOT}")
    else:
        src = ["--random", "6", "--seed", "99"]
        print("  Using random demo processes")

    result = subprocess.run(
        [sys.executable, SCHEDULER] + src +
        ["--quantum", "2", "--outdir", SCREENSHOTS],
        capture_output=True,
        text=True,
    )

    print(result.stdout)
    if result.returncode != 0:
        print(f"  [WARN] scheduler stderr:\n{result.stderr}")

    # Parse metrics from stdout for the report
    metrics = parse_scheduler_output(result.stdout)
    return metrics


def parse_scheduler_output(output: str) -> dict:
    """Extract algorithm comparison metrics from scheduler stdout."""
    metrics = {}
    current_algo = None
    for line in output.splitlines():
        if "Algorithm:" in line:
            current_algo = line.split("Algorithm:")[-1].strip()
            metrics[current_algo] = {}
        if current_algo and "Avg WT:" in line:
            try:
                parts = line.replace("|", "").split()
                # Format: Avg WT: X  Avg TAT: Y  Avg RT: Z
                metrics[current_algo]["avg_wt"]  = float(parts[parts.index("WT:") + 1])
                metrics[current_algo]["avg_tat"] = float(parts[parts.index("TAT:") + 1])
                metrics[current_algo]["avg_rt"]  = float(parts[parts.index("RT:") + 1])
            except (ValueError, IndexError):
                pass
        if current_algo and "CPU Util:" in line:
            try:
                parts = line.replace("|", "").split()
                idx = parts.index("Util:")
                metrics[current_algo]["cpu_util"] = float(parts[idx + 1].rstrip("%"))
            except (ValueError, IndexError):
                pass
    return metrics


# ─────────────────────────────────────────────────────────────────────────────
# Step 5 — Generate simulation_report.json
# ─────────────────────────────────────────────────────────────────────────────
def write_report(metrics: dict, pcbs):
    banner("Step 5: Generating Simulation Report")

    report = {
        "timestamp":    datetime.datetime.now().isoformat(),
        "module":       "CS 2104 – Operating Systems",
        "student":      "Titus Nyoni",
        "simulator":    "EduOS v1.0",
        "pcb_count":    len(pcbs) if pcbs else 0,
        "algorithms":   metrics,
        "charts": {
            "gantt_fcfs":       f"{SCREENSHOTS}/gantt_FCFS.png",
            "gantt_sjf":        f"{SCREENSHOTS}/gantt_SJF.png",
            "gantt_priority":   f"{SCREENSHOTS}/gantt_Priority.png",
            "gantt_rr":         f"{SCREENSHOTS}/gantt_RRq2.png",
            "comparison":       f"{SCREENSHOTS}/comparison_all_algorithms.png",
        }
    }

    with open(REPORT_PATH, "w") as f:
        json.dump(report, f, indent=2)

    print(f"  ✓ Report written → {REPORT_PATH}")
    print(f"\n  Summary:")
    for algo, data in metrics.items():
        print(f"    {algo:<18} WT={data.get('avg_wt','?'):<8} "
              f"TAT={data.get('avg_tat','?'):<8} "
              f"CPU={data.get('cpu_util','?')}%")


# ─────────────────────────────────────────────────────────────────────────────
# Main
# ─────────────────────────────────────────────────────────────────────────────
def main():
    print("\n╔══════════════════════════════════════════╗")
    print("║      EduOS Main Controller               ║")
    print("║      CS 2104 – Operating Systems         ║")
    print("╚══════════════════════════════════════════╝")

    start = time.time()

    # Step 1: Build
    build_c_binary()

    # Step 2: Run C
    run_c_simulator()

    # Step 3: Load snapshot
    pcbs = load_pcb_snapshot()

    # Step 4: Run scheduler
    metrics = run_scheduler(pcbs)

    # Step 5: Write report
    write_report(metrics, pcbs)

    elapsed = time.time() - start
    print(f"\n╔══════════════════════════════════════════╗")
    print(f"║  EduOS simulation complete ({elapsed:.1f}s)       ║")
    print(f"╚══════════════════════════════════════════╝\n")


if __name__ == "__main__":
    main()
