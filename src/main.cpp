#include "simview.hpp"
#include <raylib.h>

int main() {
  int height = 800;
  int width = 1280;
  InitWindow(width, height, "Scheduling simulation");
  float viewRatio = 0.6666;

  SimView simView(0, 0, 1280 * viewRatio, height);

  std::vector<std::tuple<long, long, long>> paramVector = {{6000, 500, 0},
                                                           {8000, 1000, 0}};

  simView.initTasks(paramVector);
  SetTargetFPS(60);
  int frame = 0;
  while (!WindowShouldClose()) {
    if (frame == 300) {
      simView.initTasks(paramVector);
      frame = 0;
    }
    simView.advanceState();
    BeginDrawing();
    simView.draw();
    EndDrawing();
    frame++;
  }
  CloseWindow();
}