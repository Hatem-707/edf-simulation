#pragma once

enum class ProcType {
  procA = 0,
  procB,
  procC,
};

enum class EventType { start = 0, complete, preempt, missed, initilize };

enum class SchedulingAlgo { EDF = 0, RMS };

class Event {
public:
  EventType type;
  ProcType proc;
  long timeSince = 0;
  Event(EventType type, ProcType proc) : type(type), proc(proc) {}
};
