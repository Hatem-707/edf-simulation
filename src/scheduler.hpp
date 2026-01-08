// scheduler.hpp - declarations for Task and Scheduler
#pragma once

#include "process.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <tuple>
#include <vector>

enum class TaskStatus { uninitialized = 0, waiting, running, completed };

enum class Interrupt { taskInit = 0, taskComplete, taskRestart, taskAdded };

class Task {
  int id;
  TaskStatus status = TaskStatus::uninitialized;
  Interrupt onWake = Interrupt::taskInit;
  std::chrono::milliseconds period;
  std::chrono::milliseconds duration;
  std::chrono::milliseconds runTime{0};
  std::chrono::steady_clock::time_point nextInterrupt;
  std::chrono::steady_clock::time_point deadline;
  std::chrono::steady_clock::time_point refPoint;

public:
  Task(long period, long duration,
       std::chrono::steady_clock::time_point nextInterrupt, int id);
  void run(std::chrono::steady_clock::duration duration);
  friend class Scheduler;
};

class Scheduler {
  std::vector<Task> tasks;
  std::mutex queMTX;
  std::condition_variable queCV;
  std::vector<std::tuple<long, long, long, int>> incoming;
  std::chrono::steady_clock timer;
  std::chrono::steady_clock::time_point startTime;
  std::chrono::steady_clock::time_point latestCP;
  std::optional<int> runTaskIndex;
  std::optional<std::tuple<int, Interrupt>> firedInterrupt;

  SchedulingAlgo algo = SchedulingAlgo::EDF;

  void handleTaskQ();
  void addTask(std::tuple<long, long, long, int> &taskParam);
  std::tuple<std::chrono::steady_clock::time_point, int, Interrupt>
  nextInterrupt();
  void handleInterrupt(std::tuple<int, Interrupt> firedInterrupt);
  void selectRunner();

public:
  std::function<void(Event)> eventInterface;
  std::atomic<bool> running{true};
  Scheduler(SchedulingAlgo algo = SchedulingAlgo::EDF);
  ~Scheduler();
  void initTasks(std::vector<std::tuple<long, long, long>> paramVector);
  void stop();
  void loop();
};
