/*
 CPU Task Scheduling Simulator
 Algorithms: FCFS, SJF, Round Robin, Priority Scheduling
 Features: Gantt chart, per-job metrics, aggregate statistics
 */
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <memory>
#include <functional>
#include <sstream>

//  Enums
enum class JobState { WAITING, READY, RUNNING, COMPLETED };

//  Job
class Job {
public:
    static int id_counter;

    int         id;
    std::string name;
    int         arrival_time;   // in seconds
    int         burst_time;     // in seconds
    int         priority;       // lower = higher priority
    int         remaining_time;

    // Computed results
    int         start_time      = -1;
    int         completion_time = -1;
    int         waiting_time    = 0;
    int         turnaround_time = 0;
    int         response_time   = -1;

    JobState    state = JobState::WAITING;

    Job(const std::string& name, int arrival, int burst, int priority = 0)
        : id(++id_counter), name(name),
          arrival_time(arrival), burst_time(burst),
          priority(priority), remaining_time(burst) {}

    void computeMetrics() {
        if (completion_time < 0)
            throw std::logic_error("Job " + name + " has not completed yet.");
        turnaround_time = completion_time - arrival_time;
        waiting_time    = turnaround_time - burst_time;
        if (response_time < 0) response_time = waiting_time; // fallback
    }

    void print(int nameWidth = 8) const {
        std::cout
            << std::left  << std::setw(nameWidth) << name
            << std::right << std::setw(6)  << arrival_time
                          << std::setw(8)  << burst_time
                          << std::setw(10) << priority
                          << std::setw(12) << completion_time
                          << std::setw(12) << turnaround_time
                          << std::setw(12) << waiting_time
                          << std::setw(13) << response_time
            << "\n";
    }
};

int Job::id_counter = 0;


//  GanttEntry  –  one slice on the Gantt chart
struct GanttEntry {
    std::string label;
    int         start;
    int         end;
};

//  Statistics helper
struct SchedulerStats {
    double avg_waiting_time;
    double avg_turnaround_time;
    double avg_response_time;
    double cpu_utilization;  // percentage
    int    total_time;

    void print() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n  Avg Waiting Time    : " << avg_waiting_time    << " s\n"
                  << "  Avg Turnaround Time : " << avg_turnaround_time  << " s\n"
                  << "  Avg Response Time   : " << avg_response_time    << " s\n"
                  << "  CPU Utilisation     : " << cpu_utilization      << " %\n"
                  << "  Total Makespan      : " << total_time           << " s\n";
    }
};


//  Abstract base: Scheduler
class Scheduler {
protected:
    std::vector<std::shared_ptr<Job>> jobs;
    std::vector<GanttEntry>           gantt;
    std::string                       algorithm_name;

    // Reset mutable fields so the same job list can be re-used
    void resetJobs() {
        for (auto& j : jobs) {
            j->start_time      = -1;
            j->completion_time = -1;
            j->waiting_time    = 0;
            j->turnaround_time = 0;
            j->response_time   = -1;
            j->remaining_time  = j->burst_time;
            j->state           = JobState::WAITING;
        }
        gantt.clear();
    }

    SchedulerStats computeStats() const {
        int total_burst = 0, total_idle = 0;
        for (auto& j : jobs) total_burst += j->burst_time;

        int makespan = jobs.empty() ? 0 :
            (*std::max_element(jobs.begin(), jobs.end(),
                [](auto& a, auto& b){ return a->completion_time < b->completion_time; }))
                ->completion_time;

        // Idle time = makespan – sum of burst times that actually ran
        // (approximated here as makespan – total_burst when no overlap)
        total_idle = makespan - total_burst;
        if (total_idle < 0) total_idle = 0;

        double awt = 0, att = 0, art = 0;
        for (auto& j : jobs) {
            awt += j->waiting_time;
            att += j->turnaround_time;
            art += j->response_time;
        }
        int n = static_cast<int>(jobs.size());
        return {
            awt / n, att / n, art / n,
            100.0 * total_burst / std::max(makespan, 1),
            makespan
        };
    }

    void printHeader() const {
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n"
                  << "║  Algorithm: " << std::left << std::setw(50) << algorithm_name << "║\n"
                  << "╚══════════════════════════════════════════════════════════════╝\n";

        std::cout << std::left << std::setw(8) << "Job"
                  << std::right
                  << std::setw(6)  << "Arr"
                  << std::setw(8)  << "Burst"
                  << std::setw(10) << "Priority"
                  << std::setw(12) << "Finish"
                  << std::setw(12) << "TAT"
                  << std::setw(12) << "Wait"
                  << std::setw(13) << "Response"
                  << "\n"
                  << std::string(81, '-') << "\n";
    }

    void printGantt() const {
        if (gantt.empty()) return;
        std::cout << "\n  Gantt Chart:\n  ";

        // Top bar
        for (auto& e : gantt)
            std::cout << "|" << std::setw(std::max(3, (int)e.label.size() + 2))
                      << std::left << (" " + e.label);
        std::cout << "|\n  ";

        // Time stamps
        int last = gantt.front().start;
        std::cout << last;
        for (auto& e : gantt) {
            int width = std::max(3, (int)e.label.size() + 2);
            std::cout << std::right << std::setw(width + 1) << e.end;
            last = e.end;
        }
        std::cout << "\n";
    }

public:
    explicit Scheduler(const std::string& name) : algorithm_name(name) {}
    virtual ~Scheduler() = default;

    void addJob(std::shared_ptr<Job> job) {
        jobs.push_back(std::move(job));
    }

    virtual void run() = 0;

    void report() {
        printHeader();
        for (auto& j : jobs) j->print();
        printGantt();
        auto stats = computeStats();
        std::cout << "\n  ── Summary ──";
        stats.print();
        std::cout << std::string(81, '=') << "\n";
    }
};


//  FCFS Scheduler


class FCFSScheduler : public Scheduler {
public:
    FCFSScheduler() : Scheduler("First Come First Serve (FCFS)") {}

    void run() override {
        resetJobs();
        // Sort by arrival time
        std::sort(jobs.begin(), jobs.end(),
            [](auto& a, auto& b){ return a->arrival_time < b->arrival_time; });

        int current_time = 0;
        for (auto& j : jobs) {
            int start = std::max(current_time, j->arrival_time);
            if (start > current_time)
                gantt.push_back({"IDLE", current_time, start});

            j->start_time      = start;
            j->response_time   = start - j->arrival_time;
            j->completion_time = start + j->burst_time;
            j->state           = JobState::COMPLETED;
            j->computeMetrics();

            gantt.push_back({j->name, start, j->completion_time});
            current_time = j->completion_time;
        }
    }
};


//  SJF Non-Preemptive Scheduler
class SJFScheduler : public Scheduler {
public:
    SJFScheduler() : Scheduler("Shortest Job First – Non-Preemptive (SJF)") {}

    void run() override {
        resetJobs();

        std::vector<std::shared_ptr<Job>> pending = jobs;
        std::vector<std::shared_ptr<Job>> ready;
        int current_time = 0;
        int completed    = 0;
        int n            = static_cast<int>(jobs.size());

        while (completed < n) {
            // Move arrived jobs into the ready queue
            for (auto it = pending.begin(); it != pending.end(); ) {
                if ((*it)->arrival_time <= current_time) {
                    ready.push_back(*it);
                    it = pending.erase(it);
                } else ++it;
            }

            if (ready.empty()) {
                // CPU idle: jump to the next arrival
                int next = (*std::min_element(pending.begin(), pending.end(),
                    [](auto& a, auto& b){ return a->arrival_time < b->arrival_time; }))
                    ->arrival_time;
                gantt.push_back({"IDLE", current_time, next});
                current_time = next;
                continue;
            }

            // Pick the shortest job
            auto shortest = std::min_element(ready.begin(), ready.end(),
                [](auto& a, auto& b){ return a->burst_time < b->burst_time; });
            auto j = *shortest;
            ready.erase(shortest);

            j->start_time      = current_time;
            j->response_time   = current_time - j->arrival_time;
            j->completion_time = current_time + j->burst_time;
            j->state           = JobState::COMPLETED;
            j->computeMetrics();

            gantt.push_back({j->name, current_time, j->completion_time});
            current_time = j->completion_time;
            ++completed;
        }
    }
};


//  Round Robin Scheduler
class RoundRobinScheduler : public Scheduler {
    int quantum;
public:
    explicit RoundRobinScheduler(int q = 3)
        : Scheduler("Round Robin (quantum = " + std::to_string(q) + "s)"), quantum(q) {}

    void run() override {
        resetJobs();

        // Sort arrivals
        std::vector<std::shared_ptr<Job>> sorted = jobs;
        std::sort(sorted.begin(), sorted.end(),
            [](auto& a, auto& b){ return a->arrival_time < b->arrival_time; });

        std::queue<std::shared_ptr<Job>> rq;
        int current_time = sorted.front()->arrival_time;
        size_t idx = 0;

        // Seed the queue with jobs that arrive at time 0
        while (idx < sorted.size() && sorted[idx]->arrival_time <= current_time) {
            sorted[idx]->state = JobState::READY;
            rq.push(sorted[idx++]);
        }

        while (!rq.empty()) {
            auto j = rq.front(); rq.pop();

            if (j->response_time < 0)
                j->response_time = current_time - j->arrival_time;
            if (j->start_time < 0)
                j->start_time = current_time;

            int run_for = std::min(quantum, j->remaining_time);
            gantt.push_back({j->name, current_time, current_time + run_for});
            current_time    += run_for;
            j->remaining_time -= run_for;

            // Enqueue newly arrived jobs before re-queueing the current one
            while (idx < sorted.size() && sorted[idx]->arrival_time <= current_time) {
                sorted[idx]->state = JobState::READY;
                rq.push(sorted[idx++]);
            }

            if (j->remaining_time > 0) {
                rq.push(j); // not done yet
            } else {
                j->completion_time = current_time;
                j->state           = JobState::COMPLETED;
                j->computeMetrics();
            }

            // If queue is empty but jobs still pending, advance time
            if (rq.empty() && idx < sorted.size()) {
                int next = sorted[idx]->arrival_time;
                gantt.push_back({"IDLE", current_time, next});
                current_time = next;
                while (idx < sorted.size() && sorted[idx]->arrival_time <= current_time) {
                    sorted[idx]->state = JobState::READY;
                    rq.push(sorted[idx++]);
                }
            }
        }
    }
};


//  Priority Scheduler (Non-Preemptive)
class PriorityScheduler : public Scheduler {
public:
    PriorityScheduler() : Scheduler("Priority Scheduling – Non-Preemptive (lower = higher priority)") {}

    void run() override {
        resetJobs();

        std::vector<std::shared_ptr<Job>> pending = jobs;
        std::vector<std::shared_ptr<Job>> ready;
        int current_time = 0, completed = 0;
        int n = static_cast<int>(jobs.size());

        while (completed < n) {
            for (auto it = pending.begin(); it != pending.end(); ) {
                if ((*it)->arrival_time <= current_time) {
                    ready.push_back(*it);
                    it = pending.erase(it);
                } else ++it;
            }

            if (ready.empty()) {
                int next = (*std::min_element(pending.begin(), pending.end(),
                    [](auto& a, auto& b){ return a->arrival_time < b->arrival_time; }))
                    ->arrival_time;
                gantt.push_back({"IDLE", current_time, next});
                current_time = next;
                continue;
            }

            auto highest = std::min_element(ready.begin(), ready.end(),
                [](auto& a, auto& b){ return a->priority < b->priority; });
            auto j = *highest;
            ready.erase(highest);

            j->start_time      = current_time;
            j->response_time   = current_time - j->arrival_time;
            j->completion_time = current_time + j->burst_time;
            j->state           = JobState::COMPLETED;
            j->computeMetrics();

            gantt.push_back({j->name, current_time, j->completion_time});
            current_time = j->completion_time;
            ++completed;
        }
    }
};


//  TaskManager  –  orchestrates everything
class TaskManager {
    std::vector<std::shared_ptr<Job>>       job_pool;
    std::vector<std::unique_ptr<Scheduler>> schedulers;

public:
    void addJob(const std::string& name, int arrival, int burst, int priority = 0) {
        job_pool.push_back(std::make_shared<Job>(name, arrival, burst, priority));
    }

    void registerScheduler(std::unique_ptr<Scheduler> s) {
        // Give every scheduler a fresh copy of the same jobs
        for (auto& j : job_pool) s->addJob(j);
        schedulers.push_back(std::move(s));
    }

    void runAll() {
        if (job_pool.empty()) {
            std::cerr << "[TaskManager] No jobs registered.\n";
            return;
        }
        for (auto& s : schedulers) {
            s->run();
            s->report();
        }
    }
};


//  main
#ifndef TESTING
int main() {
    std::cout << "\n╔══════════════════════════════════════╗\n"
              << "║    CPU Scheduling Simulator v1.0     ║\n"
              << "╚══════════════════════════════════════╝\n\n";

    TaskManager manager;

    //         name      arrival  burst  priority
    manager.addJob("Alpha",    0,      5,     2);
    manager.addJob("Beta",     3,      3,     1);
    manager.addJob("Gamma",   10,      8,     4);
    manager.addJob("Delta",   11,      6,     3);
    manager.addJob("Epsilon", 20,      2,     1);

    manager.registerScheduler(std::make_unique<FCFSScheduler>());
    manager.registerScheduler(std::make_unique<SJFScheduler>());
    manager.registerScheduler(std::make_unique<RoundRobinScheduler>(4));
    manager.registerScheduler(std::make_unique<PriorityScheduler>());

    manager.runAll();

    return 0;
}
#endif
