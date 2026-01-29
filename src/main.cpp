#include "app.hpp"
#include <raylib.h>

int main() {
  int height = 800;
  int width = 1280;
  InitWindow(width, height, "Scheduling simulation");
  float viewRatio = 0.6666;

  App app(1280 * viewRatio, height);

  std::vector<std::tuple<long, long, long>> paramVector = {{5000, 1000, 0},
                                                           {6000, 800, 0},
                                                           {4000, 400, 0},
                                                           {5000, 660, 0},
                                                           {2000, 300, 0}};

  app.initTasks(paramVector);
  SetTargetFPS(60);
  int frame{0};
  while (!WindowShouldClose()) {
    if (frame == 300) {
      app.removeTasks({0, 3});
    } else if (frame == 500) {
      app.initTasks({
          {6000, 1500, 50},
          {6000, 1000, 50},
      });
    }
    app.advanceView();
    BeginDrawing();
    app.draw();
    EndDrawing();
    frame++;
  }
  CloseWindow();
}