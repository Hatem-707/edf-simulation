#pragma once

enum class ProcType {
  procA = 0,
  procB,
  procC,
};

enum class EventType { start = 0, complete, preempt, missed, initilize };

class Event {
public:
  EventType type;
  ProcType proc;
  // Time relative to the moment the event object was created/pushed
  long timeSince = 0;
  Event(EventType type, ProcType proc) : type(type), proc(proc) {}
};

enum class SchedulingAlgo { EDF = 0, RMS };
