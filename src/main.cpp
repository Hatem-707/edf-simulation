#include "simview.hpp"
#include <raylib.h>

int main() {
  int height = 800;
  int width = 1280;
  InitWindow(width, height, "Scheduling simulation");
  float viewRatio = 0.6666;

  SimView simView(0, 0, 1280 * viewRatio, height);

  std::vector<std::tuple<long, long, long, ProcType>> paramVector = {
      {2000, 500, 0, ProcType::procA},
      {5000, 1000, 1000, ProcType::procB},
      {4000, 1000, 1500, ProcType::procC}};

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