#pragma once

#include "process.hpp"
#include "scheduler.hpp"
#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <raylib.h>
#include <set>
#include <thread>
#include <tuple>
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

class TraySection {
  float height;
  float width;
  float x;
  float y;
  float innerPad = 7;
  float externalPad = 3;
  float cellWidth;
  std::shared_ptr<std::vector<std::tuple<int, bool, Color>>> procPool;

  Rectangle runingRec{606.5, 63, 150, 150};
  Rectangle mainRec;

  Rectangle cellParameters(int id);

  TraySection(
      float x, float y, float width, float height,
      std::shared_ptr<std::vector<std::tuple<int, bool, Color>>> procPool);

  void draw(int activeProc);
  void updateWait(int id, bool wait);

  friend class SimView;
};

class TimeLine {
  typedef std::tuple<float, std::optional<float>, int> TLElement;
  float x;
  float y;
  float width;
  float height;
  long timeLineDuration = 200;
  std::shared_ptr<std::vector<std::tuple<int, bool, Color>>> procPool;
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

  TimeLine(float x, float y, float width, float height,
           std::shared_ptr<std::list<Event>> events,
           std::shared_ptr<std::vector<std::tuple<int, bool, Color>>> procPool);

  void advanceState(const std::set<int> &validIDs,
                    std::map<int, int> &id2Index);

  std::pair<float, float> getPosWidth(float start, float end);
  float getLaneHeight();
  float getLaneY(int id);
  std::pair<float, float> getImgCoor();

  void drawTimeLine(std::map<int, int> &id2Index);
  void drawLogs();
  void draw(std::map<int, int> &id2Index);

  friend class SimView;
};

class SimView {
  float x;
  float y;
  float width;
  float height;
  int procNum = 0;
  std::mutex eventMTX;
  std::mutex APMTX;
  int activeProc;
  std::shared_ptr<std::list<Event>> events;
  std::shared_ptr<std::vector<std::tuple<int, bool, Color>>> procPool;
  std::set<int> validIDs;
  std::map<int, int> id2Idnex;

  TraySection tray;
  TimeLine timeline;
  Scheduler sched;
  std::jthread schedulingThread;
  void eventInterface(Event);

public:
  SimView(float x, float y, float width, float height);
  ~SimView();
  void draw();
  void advanceState();
  void initTasks(std::vector<std::tuple<long, long, long>> paramVector);
  void removeTasks(std::vector<int> tasksId);
};
