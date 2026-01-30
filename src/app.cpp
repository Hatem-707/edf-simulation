#include "app.hpp"
#include "controls.hpp"
#include "process.hpp"
#include "scheduler.hpp"
#include "view.hpp"
#include <algorithm>
#include <tuple>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

std::filesystem::path get_executable_path() {
#if defined(_WIN32)
  wchar_t path[MAX_PATH];
  GetModuleFileNameW(NULL, path, MAX_PATH);
  return std::filesystem::path(path).parent_path();

#elif defined(__linux__)
  char result[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
  return std::filesystem::path(std::string(result, (count > 0) ? count : 0))
      .parent_path();

#elif defined(__APPLE__)
  char path[1024];
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) == 0)
    return std::filesystem::path(path).parent_path();
  return "";
#else
  return "";
#endif
}
namespace {
std::array<Color, 5> procColors = {{{77, 121, 105, 255},
                                    {122, 107, 99, 255},
                                    {167, 92, 92, 255},
                                    {176, 119, 84, 255},
                                    {184, 145, 75, 255}}};
}

App::App(float width, float height)
    : execPath(get_executable_path()), view(width, height, execPath),
      controls([this](int id) { this->removeTasks({id}); },
               [this](std::pair<long, long> taskParam) {
                 this->initTasks({{taskParam.first, taskParam.second, 0}});
               },
               [this](SchedulingAlgo newAlg) { this->editAlgo(newAlg); },
               execPath),
      sched(SchedulingAlgo::EDF,
            [this](Event e) { this->view.eventInterface(e); }),
      schedT([this]() { this->sched.loop(); }) {}

App::~App() { sched.stop(); }

void App::draw() {
  view.draw();
  controls.draw();
}

void App::advanceView() {
  view.advanceState();
  controls.handleInput();
}

void App::removeTasks(std::vector<int> tasksId) {
  view.removeTasks(tasksId);

  std::jthread t(
      [this](std::vector<int> tasksId) { this->sched.removeTasks(tasksId); },
      tasksId);

  std::erase_if(controls.cards, [tasksId](TaskCard t) {
    return std::find(tasksId.begin(), tasksId.end(), t.id) != tasksId.end();
  });
  void setInView();
}

void App::initTasks(std::vector<std::tuple<long, long, long>> paramVector) {
  view.initTasks(paramVector);

  std::jthread t(
      [this](std::vector<std::tuple<long, long, long>> paramVector) {
        this->sched.initTasks(paramVector);
      },
      paramVector);
  for (const auto &[period, duration, _] : paramVector) {
    controls.cards.emplace_back(nextTaskId, period, duration,
                                procColors[nextTaskId % 5],
                                controls.deleteIcon);
    nextTaskId++;
  }
  controls.setInView();
}

void App::editAlgo(SchedulingAlgo newAlgo) { sched.assignAlgo(newAlgo); }
