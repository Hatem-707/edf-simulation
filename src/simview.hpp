#pragma once

#include "process.hpp"
#include "scheduler.hpp"
#include <array>
#include <filesystem>
#include <list>
#include <memory>
#include <optional>
#include <raylib.h>
#include <thread>
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

class TraySection {
  float height;
  float width;
  float x;
  float y;
  float padding = 5;
  float innerPad = 7;
  float externalPad = 3;
  float cellWidth;
  std::shared_ptr<int> activeProcIndex;
  std::shared_ptr<std::array<std::tuple<ProcType, Color, bool>, 3>>
      procInterface;

  Rectangle runingRec{606.5, 63, 150, 150};
  Rectangle mainRec;

  Rectangle cellParameters(ProcType type);

  TraySection(float x, float y, float width, float height,
              std::shared_ptr<int> activeProcIndex,
              std::shared_ptr<std::array<std::tuple<ProcType, Color, bool>, 3>>
                  procInterface);

  void draw();
  void updateWait(ProcType proc, bool wait);

  friend class SimView;
};

class TimeLine {
  typedef std::tuple<float, std::optional<float>, ProcType> TLElement;
  float x;
  float y;
  float width;
  float height;
  long timeLineDuration = 200;
  std::shared_ptr<std::array<std::tuple<ProcType, Color, bool>, 3>>
      procInterface;
  std::shared_ptr<std::list<Event>> events;
  std::vector<TLElement> elements;
  Rectangle mainRec;
  const float logsHeight = 0.08;
  float timelineHeight;
  float divider1;
  float divider2;
  float divider3;
  float imgY;
  float imgHeight;
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
           std::shared_ptr<std::array<std::tuple<ProcType, Color, bool>, 3>>
               procInterface);

  void advanceState();

  std::tuple<float, float> getPosWidth(float start, float end);

  void drawTimeLine();
  void drawLogs();
  void draw();

  friend class SimView;
};

class SimView {
  float x;
  float y;
  float width;
  float height;
  std::mutex eventMTX;
  std::mutex APMTX;
  std::shared_ptr<int> activeProcIndex;
  std::shared_ptr<std::list<Event>> events;
  std::shared_ptr<std::array<std::tuple<ProcType, Color, bool>, 3>>
      procInterface;

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
  void
  initTasks(std::vector<std::tuple<long, long, long, ProcType>> paramVector);
};
