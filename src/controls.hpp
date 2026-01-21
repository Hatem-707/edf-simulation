#pragma once
#include <raylib.h>

class button {};

class inputField {};

class scrollButton {};

class taskCard {
  Color color;
  long id;
  long period;
  long duration;
};

class Controls {
  Color bgColor = {146, 165, 203, 255};
  Rectangle mainRec = {800, 0, 480, 800};

public:
  void draw();
};