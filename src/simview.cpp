#include "simview.hpp"
#include "process.hpp"
#include "scheduler.hpp"
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <raylib.h>
#include <thread>
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

TraySection::TraySection(
    float x, float y, float width, float height,
    std::shared_ptr<std::array<std::tuple<ProcType, Color, bool>, 3>>
        procInterface)
    : height(height), width(width),
      cellWidth((width - 2 * (externalPad + innerPad)) / 3.f), x(x), y(y),
      mainRec({x, y, width, height}), procInterface(procInterface) {}

Rectangle TraySection::cellParameters(ProcType type) {
  Rectangle cell;
  switch (type) {
  case ProcType::procA:
    cell = {x + externalPad, padding + y, cellWidth, height - 2 * padding};
    break;
  case ProcType::procB:
    cell = {cellWidth + x + innerPad + externalPad, padding + y, cellWidth,
            height - 2 * padding};
    break;
  case ProcType::procC:
    cell = {2.f * (cellWidth + innerPad) + x + externalPad, padding + y,
            cellWidth, height - 2 * padding};
    break;
  }
  return cell;
}

void TraySection::updateWait(ProcType proc, bool wait) {
  std::get<2>((*procInterface)[static_cast<int>(proc)]) = wait;
}

void TraySection::draw(int activeProc) {
  DrawRectangleRounded(mainRec, 0.25, 0, {30, 34, 42, 255});
  for (int i = 0; i < procInterface->size(); i++) {
    const auto &[type, color, waiting] = (*procInterface)[i];
    if (waiting) {
      const auto cell = cellParameters(type);
      DrawRectangleRounded(cell, 0.25, 0, color);
    }
  }
  if (activeProc != -1) {
    DrawRectangleRounded(runingRec, 0.25, 0,
                         std::get<1>((*procInterface)[activeProc]));
  } else {
    DrawRectangleRounded(runingRec, 0.25, 0, {30, 34, 42, 255});
  }
}

TimeLine::TimeLine(
    float x, float y, float width, float height,
    std::shared_ptr<std::list<Event>> events,
    std::shared_ptr<std::array<std::tuple<ProcType, Color, bool>, 3>>
        procInterface)
    : x(x), y(y), width(width), height(height), procInterface(procInterface),
      timelineHeight(height * (1 - 2. * logsHeight) / 3.),
      mainRec({x, y, width, height}), divider1(y + logsHeight * height),
      divider2(y + logsHeight * height + timelineHeight),
      divider3(y + logsHeight * height + 2 * timelineHeight),
      imgY(divider3 + timelineHeight + 0.05 * logsHeight * height),
      imgHeight(0.9 * logsHeight * height), events(events),
      execPath(get_executable_path()),

      doneSprite(LoadTexture((execPath / "assets/done.png").c_str())),
      preemptSprite(LoadTexture((execPath / "assets/preempt.png").c_str())),
      missedSprite(LoadTexture((execPath / "assets/missed.png").c_str())) {}

void TimeLine::advanceState() {
  elements.clear();
  std::array<std::optional<std::pair<TLElement, std::list<Event>::iterator>>, 3>
      elementBuffer;

  for (auto it = events->begin(); it != events->end();) {
    it->timeSince = std::min(it->timeSince + 1, timeLineDuration);
    int index = static_cast<int>(it->proc);
    if (it->type == EventType::start) {
      if (elementBuffer[index].has_value()) {
        std::cout << "New run while unterminated one exist for Process: "
                  << procTypeToString(it->proc) << std::endl;

      } else {
        elementBuffer[index] = {
            std::make_tuple(it->timeSince, std::nullopt, it->proc), it};
      }
      it++;
    } else if (it->type != EventType::missed &&
               it->type != EventType::initilize) {
      if (!elementBuffer[index].has_value()) {
        std::cout
            << "Terminating or interrupting an uninitialized run for process: "
            << procTypeToString(it->proc) << std::endl;
        it = events->erase(it);
      } else {
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

std::tuple<float, float> TimeLine::getPosWidth(float start, float end) {
  float posX = (timeLineDuration - start) / timeLineDuration;
  posX = posX * this->width + this->x;
  float width = (start - end) * this->width / timeLineDuration;
  return {posX, width};
}

void TimeLine::drawTimeLine() {
  for (const TLElement &element : elements) {
    const auto &[start, end, proc] = element;
    auto [posX, width] = getPosWidth(start, end.value());
    float posY;
    Color color = std::get<1>((*procInterface)[static_cast<int>(proc)]);
    switch (proc) {
    case ProcType::procA:
      posY = divider1;
      break;
    case ProcType::procB:
      posY = divider2;
      break;
    case ProcType::procC:
      posY = divider3;
      break;
    }
    DrawRectangle(posX, posY, width, timelineHeight, color);
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
      DrawRectangle(posX - width, divider1, width, 3 * timelineHeight, color);
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
      procInterface(
          std::make_shared<std::array<std::tuple<ProcType, Color, bool>, 3>>(
              std::array<std::tuple<ProcType, Color, bool>, 3>{
                  {{ProcType::procA, {77, 121, 105, 255}, false},
                   {ProcType::procB, {167, 92, 92, 255}, false},
                   {ProcType::procC, {184, 145, 75, 255}, false}}})),
      tray(44, 83, 320, 110, procInterface),
      timeline(44, 260, 713, 500, events, procInterface) {
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
    tray.updateWait(e.proc, true);
    break;
  case EventType::preempt: {
    std::lock_guard lk(APMTX);
    activeProc = -1;
    tray.updateWait(e.proc, true);
  } break;
  case EventType::start: {
    std::lock_guard lk(APMTX);
    activeProc = static_cast<int>(e.proc);
    tray.updateWait(e.proc, false);
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

void SimView::initTasks(
    std::vector<std::tuple<long, long, long, ProcType>> paramVector) {
  std::jthread t(
      [this](std::vector<std::tuple<long, long, long, ProcType>> paramVector) {
        this->sched.initTasks(paramVector);
      },
      paramVector);
}