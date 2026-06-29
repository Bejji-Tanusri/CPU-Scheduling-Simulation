# CPU Task Scheduling Simulator

A C++ simulation of core CPU scheduling algorithms built using Object-Oriented Programming. The simulator models job lifecycle management, computes standard OS scheduling metrics, and renders a Gantt chart for each algorithm run.

---

## Overview

The simulator implements four classical CPU scheduling algorithms on a shared job pool managed through a `TaskManager` orchestration layer. Each algorithm is encapsulated as a derived class of a polymorphic `Scheduler` base, overriding a unified `run()` interface. Jobs transition through a formal state machine (`WAITING → READY → RUNNING → COMPLETED`) and emit per-job and aggregate metrics upon completion.

### Algorithms Implemented

| Algorithm | Type | Description |
|---|---|---|
| FCFS | Non-preemptive | Processes jobs in arrival order |
| SJF | Non-preemptive | Selects shortest burst among ready jobs |
| Round Robin | Preemptive | Time-slices CPU with configurable quantum |
| Priority Scheduling | Non-preemptive | Selects highest-priority ready job (lower value = higher priority) |

### Metrics Computed

- **Waiting Time (WT)** — time a job spends in the ready queue
- **Turnaround Time (TAT)** — total time from arrival to completion
- **Response Time (RT)** — time from arrival to first CPU allocation
- **CPU Utilisation (%)** — ratio of active CPU time to total makespan
- **Gantt Chart** — per-algorithm execution timeline with idle-gap annotation

---

## Project Structure

```
.
├── task_scheduler.cpp        # Full simulator source
├── task_scheduler_tests.cpp  # 12-case OUnit-style test suite
└── README.md
```

### Class Hierarchy

```
Scheduler  (abstract base)
├── FCFSScheduler
├── SJFScheduler
├── RoundRobinScheduler
└── PriorityScheduler

Job         (models a single process)
TaskManager (orchestrates job pool + scheduler dispatch)
```

---

## Setup & Compilation

### Prerequisites

- **g++** with C++17 support
- Windows: MinGW-w64 recommended

#### Install MinGW-w64 (Windows)

```powershell
winget install -e --id MSYS2.MSYS2
```

Inside the MSYS2 terminal:

```bash
pacman -S mingw-w64-x86_64-gcc
```

Add `C:\msys64\mingw64\bin` to your system `PATH`, then restart PowerShell.

Verify installation:

```powershell
g++ --version
```

---

## Running the Simulator

```powershell
# Compile
g++ -std=c++17 -Wall -o scheduler.exe task_scheduler.cpp

# Run
.\scheduler.exe
```

---

## Running the Tests

compile and run:

```powershell
# Compile
g++ -std=c++17 -DTESTING -Wall -o tests.exe task_scheduler.cpp task_scheduler_tests.cpp

# Run (redirect output for clean reading)
.\tests.exe > results.txt
notepad results.txt
```

If Unicode box-drawing characters appear garbled in PowerShell, set UTF-8 encoding first:

```powershell
$OutputEncoding = [Console]::OutputEncoding = [Text.Encoding]::UTF8
.\tests.exe
```

---

## Test Case Results

The suite contains **12 test cases** covering correctness, edge cases, and algorithm-specific behaviour. Sample results:

---

### TC-01 — Single Job (All Algorithms)

**Input:** 1 job, arrival=0, burst=5

| Metric | Expected | Result |
|---|---|---|
| Completion Time | 5 | ✅ PASS |
| Turnaround Time | 5 | ✅ PASS |
| Waiting Time | 0 | ✅ PASS |
| Response Time | 0 | ✅ PASS |

> Validates baseline correctness across all four algorithms.

---

### TC-03 — SJF Reordering by Burst

**Input:** J1(burst=4), J2(burst=6), J3(burst=3) — all arrive at t=0

**Expected execution order:** J3 → J1 → J2

| Job | Completion Time | Waiting Time | Result |
|---|---|---|---|
| J3 (burst=3) | 3 | 0 | ✅ PASS |
| J1 (burst=4) | 7 | 3 | ✅ PASS |
| J2 (burst=6) | 13 | 7 | ✅ PASS |

> Confirms SJF correctly reorders jobs by burst length, not insertion order.

---

### TC-05 — Round Robin Preemption (q=2)

**Input:** J1(burst=5), J2(burst=3) — both arrive at t=0, quantum=2

**Gantt:** `J1[0-2] → J2[2-4] → J1[4-6] → J2[6-7] → J1[7-8]`

| Job | Completion Time | Waiting Time | Response Time | Result |
|---|---|---|---|---|
| J2 | 7 | 4 | 2 | ✅ PASS |
| J1 | 8 | 3 | 0 | ✅ PASS |

> Validates time-slice preemption and correct response-time tracking per job.

---

### TC-04 — CPU Idle Gap (FCFS)

**Input:** J1(arrival=0, burst=3), J2(arrival=10, burst=4)

**Gantt:** `J1[0-3] → IDLE[3-10] → J2[10-14]`

| Job | Completion Time | Waiting Time | Result |
|---|---|---|---|
| J1 | 3 | 0 | ✅ PASS |
| J2 | 14 | 0 | ✅ PASS |

> Confirms idle gaps are correctly detected and skipped; late-arriving jobs incur no queue wait.

---

### TC-06 — Priority Scheduling Order

**Input:** P1(priority=3, burst=4), P2(priority=1, burst=6), P3(priority=2, burst=3) — all arrive t=0

**Expected execution order:** P2 → P3 → P1

| Job | Completion Time | Waiting Time | Result |
|---|---|---|---|
| P2 (priority=1) | 6 | 0 | ✅ PASS |
| P3 (priority=2) | 9 | 6 | ✅ PASS |
| P1 (priority=3) | 13 | 9 | ✅ PASS |

> Confirms strict priority ordering regardless of insertion sequence.

---

### Full Suite Summary

```
Results:  73 passed,  0 failed  (total: 73)
```

---

## Sample Simulator Output (FCFS)

```
Algorithm: First Come First Serve (FCFS)
-----------------------------------------------------------------
Job     Arr   Burst  Priority      Finish         TAT        Wait    Response
Alpha     0       5         2           5           5           0           0
Beta      3       3         1           8           5           2           2
Gamma    10       8         4          18           8           0           0
Delta    11       6         3          24          13           7           7
Epsilon  20       2         1          26           6           4           4

Gantt Chart:
  |J1    |J2    |IDLE  |J3    |J4    |J5    |
  0      5      8      10     18     24     26

── Summary ──
  Avg Waiting Time    : 2.60 s
  Avg Turnaround Time : 7.40 s
  Avg Response Time   : 2.60 s
  CPU Utilisation     : 92.31 %
  Total Makespan      : 26 s
```

