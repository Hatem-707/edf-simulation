#include "scheduler.hpp"
#include "process.hpp"
#include <algorithm>
#include <chrono>
#include <iterator>
#include <mutex>
#include <ranges>

Task::Task(long period, long duration,
           std::chrono::steady_clock::time_point nextInterrupt, int id)
    : duration(std::chrono::milliseconds(duration)),
      period(std::chrono::milliseconds(period)), nextInterrupt(nextInterrupt),
      id(id) {}

void Task::run(std::chrono::steady_clock::duration duration) {
  runTime += std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}

Scheduler::Scheduler(SchedulingAlgo algo, std::function<void(Event)> interface)
    : startTime(timer.now()), algo(algo), eventInterface(interface) {}

Scheduler::~Scheduler() { this->stop(); }

void Scheduler::stop() {
  running = false;
  CV.notify_all();
}

void Scheduler::addTask(std::tuple<long, long, long, int> &taskParam) {
  const auto &[period, duration, delay, id] = taskParam;
  tasks.insert(
      {id,
       {period, duration, timer.now() + std::chrono::milliseconds(delay), id}});
}

void Scheduler::deleteTask(int id) { tasks.erase(id); }

void Scheduler::handleInterface() {
  {
    std::lock_guard lk(interfaceMTX);

    if (!incoming.empty()) {
      for (auto param : incoming) {
        addTask(param);
      }
      incoming.clear();
    }

    if (!tasksToRemove.empty()) {
      for (int id : tasksToRemove) {
        deleteTask(id);
      }
      tasksToRemove.clear();
    }

    if (runTaskIndex && !tasks.contains(runTaskIndex.value())) {
      runTaskIndex.reset();
    }
    if (algoBuf) {
      algo = *algoBuf;
      algoBuf.reset();
    }
  }
}

void Scheduler::initTasks(
    std::vector<std::tuple<long, long, long>> paramVector) {
  {
    std::lock_guard lk(interfaceMTX);
    for (int i = 0; i < paramVector.size(); i++) {
      const auto &[period, duration, delay] = paramVector[i];
      incoming.emplace_back(period, duration, delay, nextId);
      nextId++;
    }
  }
  CV.notify_one();
}

std::tuple<std::chrono::steady_clock::time_point, int, Interrupt>
Scheduler::nextInterrupt() {
  if (tasks.empty()) {
    return {timer.now() + std::chrono::days(2), -1, Interrupt::taskInit};
  }

  const auto it = std::ranges::min_element(
      tasks, {}, [](const auto t) { return t.second.nextInterrupt; });
  const auto &[id, task] = *it;
  return {task.nextInterrupt, id, task.onWake};
}

void Scheduler::handleInterrupt(std::tuple<int, Interrupt> firedInterrupt) {
  const auto &[id, interrupt] = firedInterrupt;

  switch (interrupt) {
  case Interrupt::taskInit: {
    if (!tasks.contains(id)) {
      return;
    }
    Task &t = tasks.at(id);
    t.status = TaskStatus::waiting;
    t.refPoint = timer.now();
    t.deadline = timer.now() + t.period;
    if (eventInterface) {
      eventInterface({EventType::initialize, t.id});
    }
    t.nextInterrupt = t.deadline;
    t.onWake = Interrupt::taskRestart;
  } break;
  case Interrupt::taskRestart: {
    if (!tasks.contains(id)) {
      return;
    }
    Task &t = tasks.at(id);
    t.refPoint = timer.now();
    if (t.status != TaskStatus::completed) {
      if (t.status == TaskStatus::running) {
        runTaskIndex.reset();
      }
      if (eventInterface) {
        eventInterface({EventType::missed, t.id});
      }
    } else {
      if (eventInterface) {
        eventInterface({EventType::resart, t.id});
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
    if (!tasks.contains(id)) {
      return;
    }
    Task &t = tasks.at(id);
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
  case Interrupt::taskEdited:
    handleInterface();
    break;
  }
}

void Scheduler::selectRunner() {
  if (tasks.empty()) {
    return;
  }
  auto filtered = tasks | std::views::filter([](const auto &t) {
                    return t.second.status == TaskStatus::waiting ||
                           t.second.status == TaskStatus::running;
                  });

  if (std::ranges::empty(filtered)) {
    runTaskIndex.reset();
    return;
  }

  auto it = std::ranges::min_element(filtered, {}, [this](const auto &t) {
    return (algo == SchedulingAlgo::EDF)
               ? ((t.second.deadline > timer.now())
                      ? t.second.deadline - timer.now()
                      : std::chrono::milliseconds::max())
               : t.second.period;
  });

  const auto &[id, task] = *it;

  if (id != runTaskIndex) {
    if (runTaskIndex) {
      Task &oldRunner = tasks.at(runTaskIndex.value());
      oldRunner.run((timer.now() - latestCP));
      oldRunner.status = TaskStatus::waiting;
      oldRunner.nextInterrupt = oldRunner.deadline;
      oldRunner.onWake = Interrupt::taskRestart;
      if (eventInterface) {
        eventInterface({EventType::preempt, oldRunner.id});
      }
    }
    runTaskIndex = id;
    Task &t = tasks.at(id);
    t.status = TaskStatus::running;
    auto remainingDuration = t.duration - t.runTime;

    if (eventInterface) {
      eventInterface({EventType::start, t.id});
    }

    if (t.deadline < remainingDuration + timer.now()) {
      t.nextInterrupt = t.deadline;
      t.onWake = Interrupt::taskRestart;
    } else {
      t.nextInterrupt = remainingDuration + timer.now();
      t.onWake = Interrupt::taskComplete;
    }
  }
}

void Scheduler::loop() {
  {
    std::unique_lock<std::mutex> lk(interfaceMTX);
    CV.wait(lk, [this]() { return !this->incoming.empty() || !running; });
  }
  if (!running) {
    return;
  }
  handleInterface();
  latestCP = timer.now();
  while (running) {
    if (firedInterrupt) {
      handleInterrupt(firedInterrupt.value());
      firedInterrupt.reset();
    }
    selectRunner();
    auto [wakeupTime, id, interrupt] = nextInterrupt();
    firedInterrupt = {id, interrupt};
    {
      latestCP = timer.now();
      if (wakeupTime < latestCP) {
        handleInterface(); // for the very unlikely event;
        continue;
      }
      std::unique_lock<std::mutex> lk(interfaceMTX);
      if (CV.wait_until(lk, wakeupTime, [this]() {
            return !this->incoming.empty() || !running ||
                   !this->tasksToRemove.empty();
          })) {
        if (!running)
          break;
        firedInterrupt = {0, Interrupt::taskEdited};
      } else {
      }
    }
  }
}

void Scheduler::removeTasks(std::vector<int> tasksId) {
  {
    std::lock_guard lk(interfaceMTX);
    tasksToRemove.insert(tasksToRemove.end(), tasksId.begin(), tasksId.end());
  }
  CV.notify_one();
}

void Scheduler::assignAlgo(SchedulingAlgo newAlgo) {
  {
    std::lock_guard lk(interfaceMTX);
    algoBuf = newAlgo;
  }
  CV.notify_one();
}
