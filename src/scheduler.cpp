#include "scheduler.hpp"
#include "process.hpp"

#include <algorithm>
#include <iterator>
#include <ranges>

Task::Task(long period, long duration,
           std::chrono::steady_clock::time_point nextInterrupt, int id)
    : duration(std::chrono::milliseconds(duration)),
      period(std::chrono::milliseconds(period)), nextInterrupt(nextInterrupt),
      id(id) {}

void Task::run(std::chrono::steady_clock::duration duration) {
  runTime += std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}

Scheduler::Scheduler(SchedulingAlgo algo)
    : startTime(timer.now()), algo(algo) {};

Scheduler::~Scheduler() { this->stop(); }

void Scheduler::stop() {
  running = false;
  queCV.notify_all();
}

void Scheduler::addTask(std::tuple<long, long, long, int> &taskParam) {
  const auto &[period, duration, delay, type] = taskParam;
  tasks.emplace_back(period, duration,
                     timer.now() + std::chrono::milliseconds(delay), type);
}

void Scheduler::handleTaskQ() {
  {
    std::lock_guard lk(queMTX);
    for (auto param : incoming) {
      addTask(param);
    }
    incoming.clear();
  }
}

void Scheduler::initTasks(
    std::vector<std::tuple<long, long, long>> paramVector) {
  {
    std::lock_guard lk(queMTX);
    for (int i = 0; i < paramVector.size(); i++) {
      const auto &[period, duration, delay] = paramVector[i];
      incoming.emplace_back(period, duration, delay, tasks.size() + i);
    }
  }
  queCV.notify_one();
}

std::tuple<std::chrono::steady_clock::time_point, int, Interrupt>
Scheduler::nextInterrupt() {
  if (tasks.empty()) {
    // Return a flag indicating nothing to schedule
    return {timer.now() + std::chrono::hours(24), -1, Interrupt::taskInit};
  }

  const auto it = std::ranges::min_element(
      tasks, {}, [](const Task &t) { return t.nextInterrupt; });
  const Task &task = *it;
  int index = std::ranges::distance(tasks.begin(), it);
  return {task.nextInterrupt, index, task.onWake};
}

void Scheduler::handleInterrupt(std::tuple<int, Interrupt> firedInterrupt) {
  const auto &[index, interrupt] = firedInterrupt;

  switch (interrupt) {
  case Interrupt::taskInit: {
    if (index < 0 || index >= tasks.size()) {
      return;
    }
    Task &t = tasks[index];
    t.status = TaskStatus::waiting;
    t.refPoint = timer.now();
    t.deadline = timer.now() + t.period;
    if (eventInterface) {
      eventInterface({EventType::initilize, t.id});
    }
    t.nextInterrupt = t.deadline;
    t.onWake = Interrupt::taskRestart;
  } break;
  case Interrupt::taskRestart: {
    if (index < 0 || index >= (int)tasks.size()) {
      return;
    }
    Task &t = tasks[index];
    t.refPoint = timer.now();
    if (t.status != TaskStatus::completed) {
      if (t.status == TaskStatus::running) {
        runTaskIndex.reset();
      }
      if (eventInterface) {
        eventInterface({EventType::missed, t.id});
      }
    }
    t.status = TaskStatus::waiting;
    t.runTime = t.runTime.zero();
    t.deadline = timer.now() + t.period;
    t.nextInterrupt = t.deadline;
    t.onWake = Interrupt::taskRestart;
    break;
  }
  case Interrupt::taskComplete: {
    if (index < 0 || index >= (int)tasks.size()) {
      return;
    }
    Task &t = tasks[index];
    t.run((timer.now() - latestCP));
    t.status = TaskStatus::completed;
    runTaskIndex.reset();
    if (eventInterface) {
      eventInterface({EventType::complete, t.id});
    }
    t.nextInterrupt = t.deadline;
    t.onWake = Interrupt::taskRestart;
    break;
  }
  case Interrupt::taskAdded:
    handleTaskQ();
    break;
  }
}

void Scheduler::selectRunner() {
  auto filtered = tasks | std::views::filter([](const Task &t) {
                    return t.status == TaskStatus::waiting ||
                           t.status == TaskStatus::running;
                  });

  if (std::ranges::empty(filtered)) {
    runTaskIndex.reset();
    return;
  }

  auto it = std::ranges::min_element(filtered, {}, [this](const Task &t) {
    return (algo == SchedulingAlgo::EDF)
               ? ((t.deadline > timer.now()) ? t.deadline - timer.now()
                                             : std::chrono::milliseconds::max())
               : t.period;
  });

  const Task &task = *it;
  int index = std::ranges::distance(tasks.begin(), it.base());

  if (index != runTaskIndex) {
    if (runTaskIndex) {
      Task &oldRunner = tasks[runTaskIndex.value()];
      oldRunner.run((timer.now() - latestCP));
      oldRunner.status = TaskStatus::waiting;
      oldRunner.nextInterrupt = oldRunner.deadline;
      oldRunner.onWake = Interrupt::taskRestart;
      if (eventInterface) {
        eventInterface({EventType::preempt, oldRunner.id});
      }
    }
    runTaskIndex = index;
    Task &t = tasks[index];
    t.status = TaskStatus::running;
    auto remaningDuration = t.duration - t.runTime;

    if (eventInterface) {
      eventInterface({EventType::start, t.id});
    }

    if (t.deadline < remaningDuration + timer.now()) {
      t.nextInterrupt = t.deadline;
      t.onWake = Interrupt::taskRestart;
    } else {
      t.nextInterrupt = remaningDuration + timer.now();
      t.onWake = Interrupt::taskComplete;
    }
  }
}

void Scheduler::loop() {
  {
    std::unique_lock<std::mutex> lk(queMTX);
    queCV.wait(lk, [this]() { return !this->incoming.empty() || !running; });
  }
  if (!running) {
    return;
  }
  handleTaskQ();
  latestCP = timer.now();
  while (running) {
    if (firedInterrupt) {
      handleInterrupt(firedInterrupt.value());
      firedInterrupt.reset();
    }
    selectRunner();
    auto [wakeupTime, index, interrupt] = nextInterrupt();
    firedInterrupt = {index, interrupt};
    {
      latestCP = timer.now();
      if (wakeupTime < latestCP) {
        continue;
      }
      std::unique_lock<std::mutex> lk(queMTX);
      if (queCV.wait_until(lk, wakeupTime, [this]() {
            return !this->incoming.empty() || !running;
          })) {
        if (!running)
          break;
        firedInterrupt = {0, Interrupt::taskAdded};
      } else {
      }
    }
  }
}
