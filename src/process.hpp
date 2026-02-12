#pragma once

enum class EventType {
  start = 0,
  complete,
  preempt,
  missed,
  initialize,
  restart
};

enum class SchedulingAlgo { EDF = 0, RMS };

class Event {
public:
  EventType type;
  int id;
  long timeSince = 0;
  Event(EventType type, int procID) : type(type), id(procID) {}
};
