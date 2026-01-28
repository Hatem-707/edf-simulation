#include "controls.hpp"
#include "process.hpp"
#include <exception>
#include <filesystem>
#include <raylib.h>
#include <string>

Button::Button(Rectangle area, Color color, std::function<void(void)> onClick)
    : area(area), color(color), onClick(onClick) {}

Button::Button(Rectangle area, Color color, std::function<void(void)> onClick,
               std::filesystem::path iconPath)
    : area(area), color(color), onClick(onClick),
      icon(LoadTexture(iconPath.string().c_str())) {}

void Button::draw() {
  DrawRectangleRounded(area, 0.15, 0, color);
  if (icon) {
    DrawTextureEx(
        *icon,
        {area.x + area.width * padRatio, area.y + area.height * padRatio}, 0,
        area.height * (1.f - 2 * padRatio) / icon->height, WHITE);
  }
}

InputCard::InputCard(Rectangle area, std::string defaultText, Color color)
    : area(area), defaultText(defaultText), color(color) {}
void InputCard::draw() {
  DrawRectangleRounded(
      {area.x + area.width * padRatio, area.y + area.height * padRatio,
       area.width * (1.f - 2 * padRatio), area.height * (1.f - 2 * padRatio)},
      0.15, 0, color);
  DrawText(defaultText.c_str(), area.x + area.width * 2 * padRatio,
           area.y + area.height * 2 * padRatio,
           static_cast<int>(area.height * 0.5 * (1.f - 2 * padRatio)), WHITE);
}

Controls::Controls(std::function<void(int)> deleteTask,
                   std::function<void(std::pair<long, long>)> addTask,
                   std::function<void(SchedulingAlgo newAlg)> switchAlg,
                   std::filesystem::path execPath)
    : deleteTaskInterface(deleteTask), addTaskInterface(addTask),
      switchAlgInterface(switchAlg),
      durationIn({mainRec.x, mainRec.y + mainRec.height * 0.8125f,
                  mainRec.width * 3 / 4, mainRec.height * 0.09375f},
                 "Task duration", darkButton),
      periodIn({mainRec.x, mainRec.y + mainRec.height * 0.90625f,
                mainRec.width * 3 / 4, mainRec.height * 0.09375f},
               "Task Period", darkButton),
      bAddTask(
          {mainRec.x + 370 * mainRec.width / 480,
           mainRec.y + mainRec.height * 675 / 800, mainRec.width * 10 / 48,
           mainRec.height / 8},
          darkButton,
          [this]() {
            try {
              long period = std::stol(this->periodIn.input);
              try {
                long duration = std::stol(this->durationIn.input);
                this->addTaskInterface({period, duration});
              } catch (const std::exception &e) {
              }
            } catch (const std::exception &e) {
            }
          },
          execPath / "assets/add.png"),
      bSwitchAlg(
          {mainRec.x + mainRec.width * 19 / 48,
           mainRec.y + 25 * mainRec.height / 800, mainRec.width * 10 / 48,
           mainRec.height / 8},
          {},
          [this]() {
            if (currentAlg == SchedulingAlgo::EDF) {
              switchAlgInterface(SchedulingAlgo::RMS);
              currentAlg = SchedulingAlgo::RMS;
            } else {
              switchAlgInterface(SchedulingAlgo::EDF);
              currentAlg = SchedulingAlgo::EDF;
            }
          },
          execPath / "assets/switch.png") {}

void Controls::draw() {
  DrawRectangleRounded(mainRec, 0.1, 0, bgColor);
  bSwitchAlg.draw();
  bAddTask.draw();
  periodIn.draw();
  durationIn.draw();
}

void Controls::shiftList(bool up) {
  if (up) {
    listInd = std::min(tasks.size(), static_cast<unsigned long>(listInd + 1));
  } else {
    listInd = std::max(0, listInd - 1);
  }
}