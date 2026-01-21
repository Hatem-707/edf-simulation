#include "controls.hpp"
#include "simview.hpp"
#include <raylib.h>

int main() {
  int height = 800;
  int width = 1280;
  InitWindow(width, height, "Scheduling simulation");
  float viewRatio = 0.6666;

  SimView simView(0, 0, 1280 * viewRatio, height);
  Controls controls;

  std::vector<std::tuple<long, long, long>> paramVector = {{5000, 1000, 50},
                                                           {6000, 1000, 50},
                                                           {4000, 1000, 50},
                                                           {4000, 1000, 50},
                                                           {4000, 1000, 2000}};

  simView.initTasks(paramVector);
  SetTargetFPS(60);
  int frame{0};
  while (!WindowShouldClose()) {
    if (frame == 300) {
      simView.removeTasks({0, 1});
    }
    simView.advanceState();
    BeginDrawing();
    simView.draw();
    controls.draw();
    EndDrawing();
    frame++;
  }
  CloseWindow();
}