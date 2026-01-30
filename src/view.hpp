#pragma once

#include "process.hpp"
#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <raylib.h>
#include <tuple>
#include <utility>
#include <vector>

class TraySection {
  float height;
  float width;
  float x;
  float y;
  float innerPad = 7;
  float externalPad = 3;
  float cellWidth;
  std::shared_ptr<std::map<int, std::pair<bool, Color>>> procPool;

  Rectangle runingRec{606, 63, 150, 150};
  Rectangle mainRec;

  Rectangle cellParameters(int id);

public:
  TraySection(float x, float y, float width, float height,
              std::shared_ptr<std::map<int, std::pair<bool, Color>>> procPool);

  void draw(int activeProc);
  void updateWait(int id, bool wait);
};

class TimeLine {
  typedef std::tuple<float, std::optional<float>, int> TLElement;
  float x;
  float y;
  float width;
  float height;
  long timeLineDuration = 200;
  std::shared_ptr<std::map<int, std::pair<bool, Color>>> procPool;
  std::shared_ptr<std::list<Event>> events;
  std::vector<TLElement> elements;
  Rectangle mainRec;
  const float logsHeight = 0.08;
  Rectangle recA;
  Rectangle recB;
  Rectangle recC;
  std::filesystem::path execPath;
  Texture doneSprite;
  Color doneColor{180, 225, 181, 150};
  Texture preemptSprite;
  Color preemptColor{249, 232, 176, 125};
  Texture missedSprite;
  Color missedColor{244, 176, 176, 125};

  std::pair<float, float> getPosWidth(float start, float end);
  float getLaneHeight();
  float getLaneY(int id);
  std::pair<float, float> getImgCoor();

public:
  TimeLine(float x, float y, float width, float height,
           std::shared_ptr<std::list<Event>> events,
           std::shared_ptr<std::map<int, std::pair<bool, Color>>> procPool,
           std::filesystem::path execPath);

  void advanceState();

  void drawTimeLine();
  void drawLogs();
  void draw();
};

class View {
  float x = 0;
  float y = 0;
  float width;
  float height;
  int procNum = 0;
  std::mutex eventMTX;
  std::mutex APMTX;
  int activeProc;
  std::shared_ptr<std::list<Event>> events;
  std::shared_ptr<std::map<int, std::pair<bool, Color>>> procPool;
  std::filesystem::path execPath;

  TraySection tray;
  TimeLine timeline;

public:
  void eventInterface(Event);
  View(float width, float height, std::filesystem::path execPath);
  void draw();
  void advanceState();
  void initTasks(std::vector<std::tuple<long, long, long>> paramVector);
  void removeTasks(std::vector<int> tasksId);
};
