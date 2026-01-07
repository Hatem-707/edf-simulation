#include "scheduler.hpp"
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

// --- Helper Functions ---
std::string procTypeToString(ProcType p) {
  switch (p) {
  case ProcType::procA:
    return "ProcA";
  case ProcType::procB:
    return "ProcB";
  case ProcType::procC:
    return "ProcC";
  default:
    return "Unknown";
  }
}

std::string algoToString(SchedulingAlgo algo) {
  return (algo == SchedulingAlgo::EDF) ? "EDF" : "RMS";
}

// Configuration to verify against
struct TaskConfig {
  double period;
  double duration;
};

// Input parameters for creating a test
struct TestScenario {
  std::string name;
  std::vector<std::tuple<long, long, long, ProcType>> tasks;
  std::map<ProcType, TaskConfig> configMap;
};

class SchedulerTest {
private:
  Scheduler scheduler;
  std::thread runnerThread;
  SchedulingAlgo algorithm;

  std::chrono::steady_clock::time_point testStart;
  std::mutex logMutex;

  struct EventLog {
    double timestampMs;
    EventType type;
    ProcType proc;
  };
  std::vector<EventLog> logs;
  TestScenario scenario;

public:
  SchedulerTest(SchedulingAlgo algo, TestScenario sc)
      : scheduler(algo), algorithm(algo), scenario(sc) {
    scheduler.eventInterface = [this](Event e) { this->logEvent(e); };
  }

  ~SchedulerTest() { stop(); }

  void logEvent(Event e) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lk(logMutex);
    double elapsed =
        std::chrono::duration<double, std::milli>(now - testStart).count();
    logs.push_back({elapsed, e.type, e.proc});
  }

  void start() {
    std::cout << "\nStarting Test: [" << scenario.name << "] using "
              << algoToString(algorithm) << "...\n";
    testStart = std::chrono::steady_clock::now();
    scheduler.initTasks(scenario.tasks);
    runnerThread = std::thread([this]() { scheduler.loop(); });
  }

  void stop() {
    if (scheduler.running) {
      scheduler.stop();
      if (runnerThread.joinable()) {
        runnerThread.join();
      }
    }
  }

  void waitAndStop(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    stop();
  }

  void verifyResults() {
    std::lock_guard<std::mutex> lk(logMutex);
    std::cout << "--------------------------------------------\n";
    std::cout << " RESULTS: " << scenario.name << " ("
              << algoToString(algorithm) << ")\n";
    std::cout << "--------------------------------------------\n";

    std::map<ProcType, std::vector<double>> completions;
    std::map<ProcType, int> missedDeadlines;
    std::map<ProcType, int> preemptions;

    // Initialize counts
    for (auto const &[p, c] : scenario.configMap) {
      missedDeadlines[p] = 0;
      preemptions[p] = 0;
    }

    // 1. Parse Logs
    for (const auto &log : logs) {
      if (log.type == EventType::complete) {
        completions[log.proc].push_back(log.timestampMs);
      } else if (log.type == EventType::missed) {
        missedDeadlines[log.proc]++;
      } else if (log.type == EventType::preempt) {
        preemptions[log.proc]++;
      }
    }

    // 2. Analyze per Process
    for (auto const &[proc, conf] : scenario.configMap) {
      std::cout << "Process: " << std::left << std::setw(6)
                << procTypeToString(proc) << " | T=" << std::setw(4)
                << conf.period << " | C=" << std::setw(4) << conf.duration
                << " | Util=" << std::fixed << std::setprecision(2)
                << (conf.duration / conf.period) * 100 << "%\n";

      // Check Misses
      if (missedDeadlines[proc] > 0) {
        std::cout << "  \033[1;31m[FAIL] Missed Deadlines: "
                  << missedDeadlines[proc] << "\033[0m\n";
      } else {
        std::cout << "  [OK]   No Misses\n";
      }

      std::cout << "  Cycles: " << completions[proc].size()
                << " | Preemptions: " << preemptions[proc] << "\n";

      std::cout << "\n";
    }
    std::cout << "============================================\n";
  }
};

// --- Scenario Generators ---

TestScenario createStandardWorkload() {
  TestScenario sc;
  sc.name = "Standard Load (Util ~65%)";
  // ProcA: 50/200, ProcB: 300/1000, ProcC: 10/100
  sc.tasks = {{200, 50, 0, ProcType::procA},
              {1000, 300, 0, ProcType::procB},
              {100, 10, 0, ProcType::procC}};
  sc.configMap[ProcType::procA] = {200.0, 50.0};
  sc.configMap[ProcType::procB] = {1000.0, 300.0};
  sc.configMap[ProcType::procC] = {100.0, 10.0};
  return sc;
}

TestScenario createOverloadWorkload() {
  TestScenario sc;
  sc.name = "Overload (Util ~125%)";
  // Deliberately impossible to schedule to test deadline miss counting
  // ProcA: 100/200 (50%), ProcB: 100/200 (50%), ProcC: 50/200 (25%) -> Total
  // 125%
  sc.tasks = {{200, 100, 0, ProcType::procA},
              {200, 100, 0, ProcType::procB},
              {200, 50, 0, ProcType::procC}};
  sc.configMap[ProcType::procA] = {200.0, 100.0};
  sc.configMap[ProcType::procB] = {200.0, 100.0};
  sc.configMap[ProcType::procC] = {200.0, 50.0};
  return sc;
}

TestScenario createRMSStressWorkload() {
  TestScenario sc;
  sc.name = "RMS Stress (Util ~88%)";
  // High utilization but theoretically possible under EDF, potentially
  // problematic for RMS due to harmonic bounds or high blocking. T=100, C=45
  // (45%) T=150, C=65 (43%) Total = 88%
  sc.tasks = {{100, 45, 0, ProcType::procA}, {150, 65, 0, ProcType::procB}};
  sc.configMap[ProcType::procA] = {100.0, 45.0};
  sc.configMap[ProcType::procB] = {150.0, 65.0};
  return sc;
}

void runTest(SchedulingAlgo algo, TestScenario sc, int durationMs) {
  SchedulerTest test(algo, sc);
  test.start();
  test.waitAndStop(durationMs);
  test.verifyResults();
  // Small pause between tests for cleanup
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

int main() {
  std::cout << "=== SCHEDULER COMPARISON SUITE ===\n";

  // 1. Standard Load - Should pass both
  auto standardParams = createStandardWorkload();
  runTest(SchedulingAlgo::EDF, standardParams, 2500);
  runTest(SchedulingAlgo::RMS, standardParams, 2500);

  // 2. Overload - Should see misses in both
  auto overloadParams = createOverloadWorkload();
  std::cout << "\n>>> NOTICE: The following tests are EXPECTED to have "
               "deadline misses.\n";
  runTest(SchedulingAlgo::EDF, overloadParams, 1500);
  runTest(SchedulingAlgo::RMS, overloadParams, 1500);

  auto rmsStressParmas = createRMSStressWorkload();
  std::cout << "\n>>> NOTICE: The following tests are EXPECTED to have "
               "deadline misses in RMS.\n";

  runTest(SchedulingAlgo::EDF, rmsStressParmas, 1500);
  runTest(SchedulingAlgo::RMS, rmsStressParmas, 1500);

  return 0;
}