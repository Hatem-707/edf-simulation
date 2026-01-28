#pragma once

#include "controls.hpp"
#include "scheduler.hpp"
#include "view.hpp"
#include <filesystem>
#include <thread>
#include <vector>

struct State;

class App {
  std::filesystem::path execPath;
  View view;
  Scheduler sched;
  Controls controls;
  std::jthread schedT;

public:
  App(float width, float height);
  ~App();
  void advanceView();
  void draw();
  void initTasks(std::vector<std::tuple<long, long, long>> paramVector);
  void removeTasks(std::vector<int> tasksId);
  void editAlgo(SchedulingAlgo newAlgo);
};
