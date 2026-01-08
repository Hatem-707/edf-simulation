#include "simview.hpp"
#include "process.hpp"
#include "scheduler.hpp"
#include <array>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <raylib.h>
#include <thread>
#include <utility>
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

std::array<Color, 5> procColors = {{{77, 121, 105, 255},
                                    {122, 107, 99, 255},
                                    {167, 92, 92, 255},
                                    {176, 119, 84, 255},
                                    {184, 145, 75, 255}}};

TraySection::TraySection(
    float x, float y, float width, float height,
    std::shared_ptr<std::vector<std::pair<int, bool>>> procPool)
    : height(height), width(width),
      cellWidth((width - 2 * (externalPad + innerPad)) / 3.f), x(x), y(y),
      mainRec({x, y, width, height}), procPool(procPool) {}

Rectangle TraySection::cellParameters(int id) {
  int numProcs = procPool->size();
  if (!numProcs) {
    return {};
  }
  if (id < 0 || id >= numProcs) {
    return {};
  }
  this->cellWidth =
      (this->width - (2 * externalPad + (numProcs - 1) * innerPad)) /
      static_cast<float>(numProcs);
  Rectangle cell;
  cell = {x + externalPad + id * (cellWidth + innerPad), y + externalPad,
          cellWidth, height - 2 * externalPad};

  return cell;
}

void TraySection::updateWait(int id, bool wait) {
  for (auto &p : *procPool) {
    if (p.first == id) {
      p.second = wait;
      return;
    }
  }
}

void TraySection::draw(int activeProc) {
  DrawRectangleRounded(mainRec, 0.25, 0, {30, 34, 42, 255});
  for (int i = 0; i < procPool->size(); i++) {
    const auto &[id, waiting] = (*procPool)[i];
    Color color = procColors[id % 5];
    if (waiting) {
      const auto cell = cellParameters(i);
      DrawRectangleRounded(cell, 0.25, 0, color);
    }
  }
  if (activeProc != -1) {
    DrawRectangleRounded(runingRec, 0.25, 0, procColors[activeProc % 5]);
  } else {
    DrawRectangleRounded(runingRec, 0.25, 0, {30, 34, 42, 255});
  }
}

TimeLine::TimeLine(float x, float y, float width, float height,
                   std::shared_ptr<std::list<Event>> events,
                   std::shared_ptr<std::vector<std::pair<int, bool>>> procPool)
    : x(x), y(y), width(width), height(height), procPool(procPool),
      mainRec({x, y, width, height}), events(events),
      execPath(get_executable_path()),

      doneSprite(LoadTexture((execPath / "assets/done.png").string().c_str())),
      preemptSprite(
          LoadTexture((execPath / "assets/preempt.png").string().c_str())),
      missedSprite(
          LoadTexture((execPath / "assets/missed.png").string().c_str())) {}

void TimeLine::advanceState() {
  elements.clear();
  int elementNumber = this->procPool->size();
  if (elementNumber <= 0) {
    return;
  }
  std::vector<std::optional<std::pair<TLElement, std::list<Event>::iterator>>>
      elementBuffer(elementNumber);

  for (auto it = events->begin(); it != events->end();) {
    it->timeSince = std::min(it->timeSince + 1, timeLineDuration);
    int index = it->id;
    if (it->type == EventType::start) {
      if (elementBuffer[index].has_value()) {
        std::cout << "New run while unterminated one exist for Process: "
                  << it->id << std::endl;

      } else {
        elementBuffer[index] = {
            std::make_tuple(it->timeSince, std::nullopt, it->id), it};
      }
      it++;
    } else if (it->type != EventType::initilize) {
      if (!elementBuffer[index].has_value() && it->type != EventType::missed) {
        std::cout
            << "Terminating or interrupting an uninitialized run for process: "
            << it->id << std::endl;
        it = events->erase(it);
      } else {
        if (!elementBuffer[index].has_value()) {
          if (it->timeSince >= timeLineDuration) {
            it = events->erase(it);
          } else {
            it++;
          }
        }
        auto &[start, end, proc] = elementBuffer[index]->first;
        end = it->timeSince;
        if (start == timeLineDuration && it->timeSince >= timeLineDuration) {
          // SAFE now because we are using std::list (iterators stay valid)
          events->erase(elementBuffer[index]->second);
          it = events->erase(it);
          elementBuffer[index].reset();
        } else {
          elements.push_back(elementBuffer[index]->first);
          elementBuffer[index].reset();
          it++;
        }
      }
    } else {
      if (it->timeSince >= timeLineDuration) {
        it = events->erase(it);
      } else {
        it++;
      }
    }
  }
  for (auto &buffered : elementBuffer) {
    if (buffered.has_value()) {
      auto &[start, end, proc] = buffered->first;
      end = 0;
      elements.push_back(buffered->first);
    }
  }
}

std::pair<float, float> TimeLine::getPosWidth(float start, float end) {
  float posX = (timeLineDuration - start) / timeLineDuration;
  posX = posX * this->width + this->x;
  float width = (start - end) * this->width / timeLineDuration;
  return {posX, width};
}

float TimeLine::getLaneHeight() {
  if (!procPool->size()) {
    return this->height * (1 - 2 * logsHeight);
  }
  return this->height * (1 - 2 * logsHeight) /
         static_cast<float>(procPool->size());
}

float TimeLine::getLaneY(int id) {
  return this->y + (logsHeight * this->height) + id * getLaneHeight();
}

std::pair<float, float> TimeLine::getImgCoor() {
  float imgY = this->y + (logsHeight * this->height) +
               (procPool->size() * getLaneHeight());
  float imgHeight = (logsHeight * this->height);
  return {imgY, imgHeight};
}

void TimeLine::drawTimeLine() {
  for (const TLElement &element : elements) {
    const auto &[start, end, id] = element;
    auto [posX, width] = getPosWidth(start, end.value());
    DrawRectangle(posX, getLaneY(id), width, getLaneHeight(),
                  procColors[id % 5]);
  }
}

void TimeLine::drawLogs() {
  for (const auto &event : (*events)) {
    Texture sprite;
    Color color;
    if (event.type == EventType::complete) {
      sprite = doneSprite;
      color = doneColor;
    } else if (event.type == EventType::preempt) {
      sprite = preemptSprite;
      color = preemptColor;
    } else if (event.type == EventType::missed) {
      sprite = missedSprite;
      color = missedColor;
    } else {
      continue;
    }
    auto [posX, width] = getPosWidth(event.timeSince, event.timeSince - 5);
    if (posX - 5 > this->x) {
      const auto &[imgY, imgHeight] = getImgCoor();
      float height = procPool->size() * getLaneHeight();
      DrawRectangle(posX - width, getLaneY(0), width, height, color);
      DrawTextureEx(sprite, {posX - width, imgY}, 0, imgHeight / sprite.height,
                    WHITE);
    }
  }
}

void TimeLine::draw() {
  DrawRectangleRounded(mainRec, 0.10, 0, {30, 34, 42, 255});
  drawTimeLine();
  drawLogs();
}

SimView::SimView(float x, float y, float width, float height)
    : x(x), y(y), width(width), height(height),
      events(std::make_shared<std::list<Event>>()),
      procPool(std::make_shared<std::vector<std::pair<int, bool>>>()),
      tray(44, 83, 320, 110, procPool),
      timeline(44, 260, 713, 500, events, procPool) {
  sched.eventInterface = [this](Event e) { this->eventInterface(e); };
  schedulingThread = std::jthread([this]() { this->sched.loop(); });
}

SimView::~SimView() { sched.stop(); }

void SimView::eventInterface(Event e) {
  {
    std::lock_guard lk(eventMTX);
    events->emplace_back(e);
  }
  switch (e.type) {
  case EventType::initilize:
    break;
  case EventType::complete: {
    std::lock_guard lk(APMTX);
    activeProc = -1;
  } break;
  case EventType::missed:
    tray.updateWait(e.id, true);
    break;
  case EventType::preempt: {
    std::lock_guard lk(APMTX);
    activeProc = -1;
    tray.updateWait(e.id, true);
  } break;
  case EventType::start: {
    std::lock_guard lk(APMTX);
    activeProc = e.id;
    tray.updateWait(e.id, false);
  } break;
  }
}

void SimView::draw() {
  ClearBackground({54, 61, 75, 255});
  {
    std::lock_guard lk(APMTX);
    tray.draw(activeProc);
  }
  timeline.draw();
}

void SimView::advanceState() {
  {
    std::lock_guard lk(eventMTX);
    timeline.advanceState();
  }
}

void SimView::initTasks(std::vector<std::tuple<long, long, long>> paramVector) {
  int start = procPool->size();
  for (int i = 0; i < paramVector.size(); ++i) {
    procPool->emplace_back(start + i, false);
  }
  std::jthread t(
      [this](std::vector<std::tuple<long, long, long>> paramVector) {
        this->sched.initTasks(paramVector);
      },
      paramVector);
}