#include "simview.hpp"
#include <raylib.h>

int main() {
  int height = 800;
  int width = 1280;
  InitWindow(width, height, "Scheduling simulation");
  float viewRatio = 0.6666;

  SimView simView(0, 0, 1280 * viewRatio, height);

  std::vector<std::tuple<long, long, long>> paramVector = {
      {2000, 700, 0},    {5000, 1200, 1000}, {5000, 1300, 600},
      {5000, 800, 1500}, {5000, 1100, 2000}, {5000, 1200, 1000},
      {5000, 1300, 600}, {5000, 800, 1500}};

  simView.initTasks(paramVector);
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    simView.advanceState();
    BeginDrawing();
    simView.draw();
    EndDrawing();
  }
  CloseWindow();
}