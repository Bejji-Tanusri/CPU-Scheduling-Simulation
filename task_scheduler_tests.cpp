/*
Test Suite for CPU Task Scheduling Simulator

Each test case covers a distinct scheduling scenario.
Expected values are hand-computed and verified inline.

To run tests:
    g++ -std=c++17 -Wall -o tests.exe task_scheduler_tests.cpp
    $OutputEncoding = [Console]::OutputEncoding = [Text.Encoding]::UTF8
    .\tests.exe

*/

#define TESTING   // suppress task_scheduler.cpp's main()
#include "task_scheduler.cpp"  

#include <cassert>
#include <iostream>
#include <string>
#include <vector>


//  Tiny test harness
static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_EQ(actual, expected, msg)                                        \
    do {                                                                        \
        ++tests_run;                                                            \
        if ((actual) == (expected)) {                                           \
            ++tests_passed;                                                     \
            std::cout << "  [PASS] " << (msg) << "\n";                        \
        } else {                                                                \
            ++tests_failed;                                                     \
            std::cout << "  [FAIL] " << (msg)                                 \
                      << "\n         expected=" << (expected)                   \
                      << "  got=" << (actual) << "\n";                         \
        }                                                                       \
    } while (0)

void printSectionHeader(const std::string& title) {
    std::cout << "\n┌─────────────────────────────────────────────┐\n"
              << "│  " << std::left << std::setw(44) << title << "│\n"
              << "└─────────────────────────────────────────────┘\n";
}


//  Helper: build a fresh scheduler + jobs,
//  run it, and return the job vector.
struct JobDef { std::string name; int arrival, burst, priority; };

// Returns the jobs after running the given scheduler type.
// SchedulerT must be default-constructible.
template<typename SchedulerT>
std::vector<std::shared_ptr<Job>> runScenario(
    const std::vector<JobDef>& defs,
    SchedulerT* sched_out = nullptr)          // optional: caller-owned ptr to peek
{
    Job::id_counter = 0;   // reset so IDs don't drift between tests

    auto sched = std::make_unique<SchedulerT>();
    std::vector<std::shared_ptr<Job>> jobs;
    for (auto& d : defs) {
        auto j = std::make_shared<Job>(d.name, d.arrival, d.burst, d.priority);
        jobs.push_back(j);
        sched->addJob(j);
    }
    sched->run();
    return jobs;
}

// Specialisation for Round Robin (needs a quantum parameter)
std::vector<std::shared_ptr<Job>> runRR(
    const std::vector<JobDef>& defs, int quantum)
{
    Job::id_counter = 0;

    auto sched = std::make_unique<RoundRobinScheduler>(quantum);
    std::vector<std::shared_ptr<Job>> jobs;
    for (auto& d : defs) {
        auto j = std::make_shared<Job>(d.name, d.arrival, d.burst, d.priority);
        jobs.push_back(j);
        sched->addJob(j);
    }
    sched->run();
    return jobs;
}


//  TC-01  Single Job
//  One job arriving at t=0, burst=5.
//  Expected (all algorithms): finish=5, TAT=5, WT=0.
void tc01_single_job() {
    printSectionHeader("TC-01  Single Job");

    std::vector<JobDef> defs = {{"J1", 0, 5, 0}};

    for (auto& label : std::vector<std::string>{"FCFS","SJF","RR","Priority"}) {
        std::vector<std::shared_ptr<Job>> jobs;
        if      (label == "FCFS")     jobs = runScenario<FCFSScheduler>(defs);
        else if (label == "SJF")      jobs = runScenario<SJFScheduler>(defs);
        else if (label == "RR")       jobs = runRR(defs, 3);
        else                          jobs = runScenario<PriorityScheduler>(defs);

        ASSERT_EQ(jobs[0]->completion_time, 5,  label + ": completion_time");
        ASSERT_EQ(jobs[0]->turnaround_time, 5,  label + ": turnaround_time");
        ASSERT_EQ(jobs[0]->waiting_time,    0,  label + ": waiting_time");
        ASSERT_EQ(jobs[0]->response_time,   0,  label + ": response_time");
    }
}


//  TC-02  All Jobs Arrive at t=0
//  FCFS must process in insertion order.
//  Hand-computed:
//    J1: finish=4,  TAT=4,  WT=0
//    J2: finish=10, TAT=10, WT=6
//    J3: finish=13, TAT=13, WT=10


void tc02_all_arrive_t0_fcfs() {
    printSectionHeader("TC-02  All Arrive t=0 – FCFS Order");

    std::vector<JobDef> defs = {
        {"J1", 0, 4, 0},
        {"J2", 0, 6, 0},
        {"J3", 0, 3, 0},
    };
    auto jobs = runScenario<FCFSScheduler>(defs);

    ASSERT_EQ(jobs[0]->completion_time,  4, "J1 finish");
    ASSERT_EQ(jobs[1]->completion_time, 10, "J2 finish");
    ASSERT_EQ(jobs[2]->completion_time, 13, "J3 finish");
    ASSERT_EQ(jobs[0]->waiting_time,     0, "J1 wait=0");
    ASSERT_EQ(jobs[1]->waiting_time,     4, "J2 wait=4");
    ASSERT_EQ(jobs[2]->waiting_time,    10, "J3 wait=10");
}


//  TC-03  SJF Reorders by Burst
//  All arrive t=0. SJF picks shortest first.
//  Sorted: J3(3) → J1(4) → J2(6)
//    J3: finish=3,  WT=0
//    J1: finish=7,  WT=3
//    J2: finish=13, WT=7


void tc03_sjf_reorder() {
    printSectionHeader("TC-03  SJF Reorders by Burst");

    std::vector<JobDef> defs = {
        {"J1", 0, 4, 0},
        {"J2", 0, 6, 0},
        {"J3", 0, 3, 0},
    };
    auto jobs = runScenario<SJFScheduler>(defs);

    // After SJF: jobs vector is still in original order; find by name.
    auto find = [&](const std::string& n) -> std::shared_ptr<Job> {
        for (auto& j : jobs) if (j->name == n) return j;
        return nullptr;
    };

    auto j1 = find("J1"), j2 = find("J2"), j3 = find("J3");

    ASSERT_EQ(j3->completion_time,  3, "SJF: J3(burst=3) finishes first");
    ASSERT_EQ(j1->completion_time,  7, "SJF: J1(burst=4) finishes second");
    ASSERT_EQ(j2->completion_time, 13, "SJF: J2(burst=6) finishes last");
    ASSERT_EQ(j3->waiting_time,     0, "SJF: J3 wait=0");
    ASSERT_EQ(j1->waiting_time,     3, "SJF: J1 wait=3");
    ASSERT_EQ(j2->waiting_time,     7, "SJF: J2 wait=7");
}


//  TC-04  CPU Idle Gap
//  Job arrives well after t=0; CPU must be idle in between.
//    J1: arrival=0, burst=3  → finish=3
//    J2: arrival=10, burst=4 → finish=14 (idle 3..10)


void tc04_idle_gap_fcfs() {
    printSectionHeader("TC-04  CPU Idle Gap (FCFS)");

    std::vector<JobDef> defs = {
        {"J1",  0, 3, 0},
        {"J2", 10, 4, 0},
    };
    auto jobs = runScenario<FCFSScheduler>(defs);

    ASSERT_EQ(jobs[0]->completion_time,  3, "J1 finishes at t=3");
    ASSERT_EQ(jobs[0]->waiting_time,     0, "J1 no waiting");
    ASSERT_EQ(jobs[1]->completion_time, 14, "J2 finishes at t=14");
    ASSERT_EQ(jobs[1]->waiting_time,     0, "J2 no waiting (arrived after idle)");
    ASSERT_EQ(jobs[1]->response_time,    0, "J2 response=0 (no queue wait)");
}


//  TC-05  Round Robin – Preemption
//  quantum=2; J1(burst=5) and J2(burst=3) both arrive t=0.
//  Timeline: J1[0-2] J2[2-4] J1[4-6] J2[6-7] J1[7-8]
//    J2: finish=7,  TAT=7, WT=4   (7-0-3=4)
//    J1: finish=8,  TAT=8, WT=3   (8-0-5=3)


void tc05_rr_preemption() {
    printSectionHeader("TC-05  Round Robin Preemption (q=2)");

    std::vector<JobDef> defs = {
        {"J1", 0, 5, 0},
        {"J2", 0, 3, 0},
    };
    auto jobs = runRR(defs, 2);

    auto find = [&](const std::string& n) -> std::shared_ptr<Job> {
        for (auto& j : jobs) if (j->name == n) return j;
        return nullptr;
    };
    auto j1 = find("J1"), j2 = find("J2");

    ASSERT_EQ(j2->completion_time, 7, "RR q=2: J2 finishes at t=7");
    ASSERT_EQ(j1->completion_time, 8, "RR q=2: J1 finishes at t=8");
    ASSERT_EQ(j2->waiting_time,    4, "RR q=2: J2 waiting=4");
    ASSERT_EQ(j1->waiting_time,    3, "RR q=2: J1 waiting=3");
    ASSERT_EQ(j1->response_time,   0, "RR q=2: J1 first scheduled at t=0");
    ASSERT_EQ(j2->response_time,   2, "RR q=2: J2 first scheduled at t=2");
}


//  TC-06  Priority Scheduling
//  All arrive t=0. Lower priority number = higher priority.
//    P1(priority=3, burst=4)
//    P2(priority=1, burst=6)   ← highest priority
//    P3(priority=2, burst=3)
//  Order: P2 → P3 → P1
//    P2: finish=6,  WT=0
//    P3: finish=9,  WT=6
//    P1: finish=13, WT=9


void tc06_priority_order() {
    printSectionHeader("TC-06  Priority Scheduling Order");

    std::vector<JobDef> defs = {
        {"P1", 0, 4, 3},
        {"P2", 0, 6, 1},
        {"P3", 0, 3, 2},
    };
    auto jobs = runScenario<PriorityScheduler>(defs);

    auto find = [&](const std::string& n) -> std::shared_ptr<Job> {
        for (auto& j : jobs) if (j->name == n) return j;
        return nullptr;
    };
    auto p1 = find("P1"), p2 = find("P2"), p3 = find("P3");

    ASSERT_EQ(p2->completion_time,  6, "Priority: P2(pri=1) runs first, finishes at 6");
    ASSERT_EQ(p3->completion_time,  9, "Priority: P3(pri=2) runs second, finishes at 9");
    ASSERT_EQ(p1->completion_time, 13, "Priority: P1(pri=3) runs last, finishes at 13");
    ASSERT_EQ(p2->waiting_time,     0, "Priority: P2 no wait");
    ASSERT_EQ(p3->waiting_time,     6, "Priority: P3 wait=6");
    ASSERT_EQ(p1->waiting_time,     9, "Priority: P1 wait=9");
}


//  TC-07  Staggered Arrivals – SJF
//  Jobs trickle in; SJF must pick the shortest
//  among the *ready* queue at each decision point.
//    J1: arr=0,  burst=8
//    J2: arr=1,  burst=4
//    J3: arr=2,  burst=2
//  At t=0 only J1 is ready → runs J1 until finish=8.
//  At t=8 both J2 and J3 are ready → SJF picks J3(burst=2).
//  Order: J1 → J3 → J2
//    J1: finish=8,  TAT=8,  WT=0
//    J3: finish=10, TAT=8,  WT=6
//    J2: finish=14, TAT=13, WT=9


void tc07_staggered_sjf() {
    printSectionHeader("TC-07  Staggered Arrivals – SJF");

    std::vector<JobDef> defs = {
        {"J1", 0, 8, 0},
        {"J2", 1, 4, 0},
        {"J3", 2, 2, 0},
    };
    auto jobs = runScenario<SJFScheduler>(defs);

    auto find = [&](const std::string& n) -> std::shared_ptr<Job> {
        for (auto& j : jobs) if (j->name == n) return j;
        return nullptr;
    };
    auto j1 = find("J1"), j2 = find("J2"), j3 = find("J3");

    ASSERT_EQ(j1->completion_time,  8, "Staggered SJF: J1 finishes at t=8");
    ASSERT_EQ(j3->completion_time, 10, "Staggered SJF: J3(shortest ready) finishes at t=10");
    ASSERT_EQ(j2->completion_time, 14, "Staggered SJF: J2 finishes at t=14");
    ASSERT_EQ(j1->waiting_time,     0, "Staggered SJF: J1 WT=0");
    ASSERT_EQ(j3->waiting_time,     6, "Staggered SJF: J3 WT=6");
    ASSERT_EQ(j2->waiting_time,     9, "Staggered SJF: J2 WT=9");
}


//  TC-08  RR Quantum Equals Burst (No Preemption)
//  If quantum >= every burst, RR == FCFS.
//    J1: arr=0, burst=3
//    J2: arr=0, burst=5
//  With q=10: J1[0-3] J2[3-8]
//    J1: finish=3, WT=0
//    J2: finish=8, WT=3


void tc08_rr_no_preemption() {
    printSectionHeader("TC-08  RR Quantum >= All Bursts → No Preemption");

    std::vector<JobDef> defs = {
        {"J1", 0, 3, 0},
        {"J2", 0, 5, 0},
    };
    auto jobs = runRR(defs, 10);

    ASSERT_EQ(jobs[0]->completion_time, 3, "RR q=10: J1 finishes at t=3");
    ASSERT_EQ(jobs[1]->completion_time, 8, "RR q=10: J2 finishes at t=8");
    ASSERT_EQ(jobs[0]->waiting_time,    0, "RR q=10: J1 WT=0");
    ASSERT_EQ(jobs[1]->waiting_time,    3, "RR q=10: J2 WT=3");
}


//  TC-09  Equal Burst Times – FCFS vs SJF
//  When all bursts are identical, SJF must produce
//  the same order and results as FCFS.


void tc09_equal_bursts() {
    printSectionHeader("TC-09  Equal Burst Times – FCFS == SJF");

    std::vector<JobDef> defs = {
        {"J1", 0, 4, 0},
        {"J2", 2, 4, 0},
        {"J3", 5, 4, 0},
    };
    auto fcfs_jobs = runScenario<FCFSScheduler>(defs);
    Job::id_counter = 0;
    auto sjf_jobs  = runScenario<SJFScheduler>(defs);

    for (int i = 0; i < 3; ++i) {
        std::string tag = "J" + std::to_string(i+1);
        ASSERT_EQ(fcfs_jobs[i]->completion_time,
                  sjf_jobs[i]->completion_time,
                  tag + ": FCFS finish == SJF finish");
        ASSERT_EQ(fcfs_jobs[i]->waiting_time,
                  sjf_jobs[i]->waiting_time,
                  tag + ": FCFS wait == SJF wait");
    }
}


//  TC-10  Priority Tie-Breaking (same priority)
//  When two jobs share a priority, the one that
//  arrived first should run first.
//    Both arrive t=0, priority=2, burst=3 and 5.
//  With FCFS-style tie-break: J1 runs before J2.


void tc10_priority_tiebreak() {
    printSectionHeader("TC-10  Priority Tie-Break by Arrival");

    std::vector<JobDef> defs = {
        {"J1", 0, 3, 2},
        {"J2", 0, 5, 2},
    };
    auto jobs = runScenario<PriorityScheduler>(defs);

    // Both have priority=2; J1 was inserted first so should run first.
    auto find = [&](const std::string& n) -> std::shared_ptr<Job> {
        for (auto& j : jobs) if (j->name == n) return j;
        return nullptr;
    };
    auto j1 = find("J1"), j2 = find("J2");

    ASSERT_EQ(j1->completion_time, 3, "Priority tie: J1 finishes at t=3");
    ASSERT_EQ(j2->completion_time, 8, "Priority tie: J2 finishes at t=8");
}


//  TC-11  Response Time Correctness (RR)
//  response_time = time from arrival to FIRST CPU slice.
//  With q=3, J1 and J2 both arrive at t=0.
//    J1 is served first → response=0
//    J2 waits one quantum → response=3


void tc11_response_time_rr() {
    printSectionHeader("TC-11  Response Time in Round Robin");

    std::vector<JobDef> defs = {
        {"J1", 0, 6, 0},
        {"J2", 0, 6, 0},
    };
    auto jobs = runRR(defs, 3);

    auto find = [&](const std::string& n) -> std::shared_ptr<Job> {
        for (auto& j : jobs) if (j->name == n) return j;
        return nullptr;
    };
    auto j1 = find("J1"), j2 = find("J2");

    ASSERT_EQ(j1->response_time, 0, "RR: J1 first response at t=0");
    ASSERT_EQ(j2->response_time, 3, "RR: J2 first response at t=3 (after 1 quantum)");
}


//  TC-12  Large Spread of Arrivals (FCFS)
//  Each job arrives exactly when the previous finishes.
//  No idle gaps; waiting_time = 0 for all.


void tc12_back_to_back_fcfs() {
    printSectionHeader("TC-12  Back-to-Back Arrivals – No Idle, No Wait");

    std::vector<JobDef> defs = {
        {"J1",  0, 5, 0},
        {"J2",  5, 4, 0},
        {"J3",  9, 3, 0},
        {"J4", 12, 6, 0},
    };
    auto jobs = runScenario<FCFSScheduler>(defs);

    int expected_finish[] = {5, 9, 12, 18};
    for (int i = 0; i < 4; ++i) {
        std::string tag = "J" + std::to_string(i+1);
        ASSERT_EQ(jobs[i]->completion_time, expected_finish[i], tag + " finish");
        ASSERT_EQ(jobs[i]->waiting_time,    0,                  tag + " WT=0");
    }
}


//  main


int main() {
    std::cout << "\n╔══════════════════════════════════════════╗\n"
              << "║    CPU Scheduler – Test Suite            ║\n"
              << "╚══════════════════════════════════════════╝\n";

    tc01_single_job();
    tc02_all_arrive_t0_fcfs();
    tc03_sjf_reorder();
    tc04_idle_gap_fcfs();
    tc05_rr_preemption();
    tc06_priority_order();
    tc07_staggered_sjf();
    tc08_rr_no_preemption();
    tc09_equal_bursts();
    tc10_priority_tiebreak();
    tc11_response_time_rr();
    tc12_back_to_back_fcfs();

    std::cout << "\n══════════════════════════════════════════════\n"
              << "  Results:  "
              << tests_passed << " passed,  "
              << tests_failed << " failed  "
              << "(total: " << tests_run << ")\n"
              << "══════════════════════════════════════════════\n\n";

    return tests_failed == 0 ? 0 : 1;
}
