#include "view.hpp"
#include "process.hpp"
#include <array>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <raylib.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

std::array<Color, 5> procColors = {{{77, 121, 105, 255},
                                    {122, 107, 99, 255},
                                    {167, 92, 92, 255},
                                    {176, 119, 84, 255},
                                    {184, 145, 75, 255}}};

TraySection::TraySection(
    float x, float y, float width, float height,
    std::shared_ptr<std::map<int, std::pair<bool, Color>>> procPool)
    : height(height), width(width),
      cellWidth((width - 2 * (externalPad + innerPad)) / 3.f), x(x), y(y),
      mainRec({x, y, width, height}), procPool(procPool) {}

Rectangle TraySection::cellParameters(int index) {
  int numProcs = procPool->size();
  if (!numProcs) {
    return {};
  }
  if (index < 0 || index >= numProcs) {
    return {};
  }
  this->cellWidth =
      (this->width - (2 * externalPad + (numProcs - 1) * innerPad)) /
      static_cast<float>(numProcs);
  Rectangle cell;
  cell = {x + externalPad + index * (cellWidth + innerPad), y + externalPad,
          cellWidth, height - 2 * externalPad};

  return cell;
}

void TraySection::updateWait(int id, bool wait) {
  if (!procPool->contains(id)) {
    return;
  }
  procPool->at(id).first = wait;
}

void TraySection::draw(int activeProc) {
  DrawRectangleRounded(mainRec, 0.25, 0, {30, 34, 42, 255});
  for (const auto &[id, p] : *procPool) {
    const auto &[waiting, color] = p;

    const auto mapIT = procPool->find(id);
    int index = std::distance(procPool->begin(), mapIT);

    if (auto cell = cellParameters(index); waiting) {
      DrawRectangleRounded(cell, 0.25, 0, color);
    } else {
      cell.x += 2;
      cell.y += 2;
      cell.height -= 4;
      cell.width -= 4;

      DrawRectangleRoundedLinesEx(cell, 0.25, 0, 2, color);
    }
  }
  if (procPool->contains(activeProc)) {
    DrawRectangleRounded(runingRec, 0.25, 0, procPool->at(activeProc).second);
    DrawText(std::to_string(activeProc).c_str(),
             runingRec.x + runingRec.width / 4.f, runingRec.y, runingRec.height,
             WHITE);
  } else {
    DrawRectangleRounded(runingRec, 0.25, 0, {30, 34, 42, 255});
  }
}

TimeLine::TimeLine(
    float x, float y, float width, float height,
    std::shared_ptr<std::list<Event>> events,
    std::shared_ptr<std::map<int, std::pair<bool, Color>>> procPool,
    std::filesystem::path pExcutable)
    : x(x), y(y), width(width), height(height), procPool(procPool),
      mainRec({x, y, width, height}), events(events), execPath(pExcutable),

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
    if (!procPool->contains(it->id)) {
      it = events->erase(it);
      continue;
    }

    it->timeSince = std::min(it->timeSince + 1, timeLineDuration);

    const auto mapIT = procPool->find(it->id);
    int index = std::distance(procPool->begin(), mapIT);
    // Events responsiple for setting the wait state only
    if (it->type == EventType::initialize || it->type == EventType::restart) {
      it = events->erase(it);
      continue;
    } else if (it->type == EventType::start) {
      if (elementBuffer[index]) {
        std::cout << "New run while unterminated one exist for Process: "
                  << it->id << std::endl;
        it = events->erase(it);
        continue;
      } else {
        elementBuffer[index] = {
            std::make_tuple(it->timeSince, std::nullopt, it->id), it};
      }
      it++;
      continue;
    } else {
      if (!elementBuffer[index] && it->type != EventType::missed) {
        std::cout
            << "Terminating or interrupting an uninitialized run for process: "
            << it->id << std::endl;
        it = events->erase(it);
        continue;
      } else if (elementBuffer[index]) {
        auto &[start, end, proc] = elementBuffer[index]->first;
        end = it->timeSince;
        if (start == timeLineDuration && it->timeSince >= timeLineDuration) {
          events->erase(elementBuffer[index]->second);
          elementBuffer[index].reset();
          it = events->erase(it);
          continue;
        } else {
          elements.push_back(elementBuffer[index]->first);
          elementBuffer[index].reset();
          it++;
          continue;
        }
      } else {
        if (it->timeSince >= timeLineDuration) {
          it = events->erase(it);
          continue;
        } else {
          it++;
          continue;
        }
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
    auto [start, end, id] = element;

    const auto mapIT = procPool->find(id);
    int index = std::distance(procPool->begin(), mapIT);

    auto [posX, width] = getPosWidth(start, end.value());
    DrawRectangle(posX, getLaneY(index), width, getLaneHeight(),
                  mapIT->second.second);
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

View::View(float width, float height, std::filesystem::path pExcutable)
    : width(width), height(height),
      events(std::make_shared<std::list<Event>>()),
      procPool(std::make_shared<std::map<int, std::pair<bool, Color>>>()),
      execPath(pExcutable), tray(44, 83, 320, 110, procPool),
      timeline(44, 260, 713, 500, events, procPool, pExcutable) {}

void View::eventInterface(Event e) {
  {
    std::lock_guard lk(eventMTX);
    if (e.type != EventType::initialize && e.type != EventType::restart) {
      events->emplace_back(e);
    }
  }
  switch (e.type) {
  case EventType::initialize:
    tray.updateWait(e.id, true);
    break;
  case EventType::restart:
    tray.updateWait(e.id, true);
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

void View::draw() {
  ClearBackground({54, 61, 75, 255});
  DrawText("Waiting Tasks", 95, 43, 30, WHITE);
  DrawText("Active Task", 590, 23, 30, WHITE);
  {
    std::lock_guard lk(APMTX);
    tray.draw(activeProc);
  }
  timeline.draw();
}

void View::advanceState() {
  {
    std::lock_guard lk(eventMTX);
    timeline.advanceState();
  }
}

void View::initTasks(std::vector<std::tuple<long, long, long>> paramVector) {
  for (int i = 0; i < paramVector.size(); ++i) {
    procPool->insert({procNum, {false, procColors[procNum % 5]}});
    procNum++;
  }
}

void View::removeTasks(std::vector<int> tasksId) {
  for (int id : tasksId) {
    procPool->erase(id);
  }
}
