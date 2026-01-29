#include "controls.hpp"
#include "process.hpp"
#include <filesystem>
#include <raylib.h>
#include <string>

Button::Button(Rectangle area, Color color, std::function<void(void)> onClick)
    : area(area), color(color), onClick(onClick) {}

Button::Button(Rectangle area, Color color, std::function<void(void)> onClick,
               Texture icon)
    : area(area), color(color), onClick(onClick), icon(icon) {}

void Button::draw() {

  DrawRectangleRounded(area, 0.15, 0, color);
  if (icon) {
    if (flip) {
      DrawTextureEx(*icon, {area.x + area.width, area.y + area.height}, 180,
                    area.height / icon->height, WHITE);
    } else {
      DrawTextureEx(*icon, {area.x, area.y}, 0, area.height / icon->height,
                    WHITE);
    }
  }
}

InputCard::InputCard(Rectangle area, std::string defaultText, Color color)
    : area(area), defaultText(defaultText), color(color) {}

void InputCard::draw() {
  DrawRectangleRounded(
      {area.x + area.width * padRatio, area.y + area.height * padRatio,
       area.width * (1.f - 2 * padRatio), area.height * (1.f - 2 * padRatio)},
      0.25, 0, color);
  DrawText(defaultText.c_str(), area.x + area.width * 2 * padRatio,
           area.y + area.height * 0.6 * (1.f - 2 * padRatio),
           static_cast<int>(area.height * 0.4 * (1.f - 2 * padRatio)), WHITE);
}

void InputCard::setActive(bool active) { this->active = active; }

TaskCard::TaskCard(int id, long period, long duration, Color color,
                   std::function<void(int)> fRemove, Texture buttonIcon)
    : remove(
          {}, BLACK, [this]() { this->fRemove(this->id); }, buttonIcon),
      id(id), duration(duration), period(period), fRemove(fRemove),
      color(color) {}

void TaskCard::draw(int index, Rectangle listRec) {
  if (index < 0 || index > 4) {
    return;
  } else {
    float cellHeight = listRec.height / 5.f;
    float offsetY = listRec.y + cellHeight * index;
    float buttonRatio = 1.f / 3.f;
    float padRatio = 0.05;
    int fontSize = 0.3 * (cellHeight * (1 - 2 * padRatio));
    float fontOffset = 0.4 * (cellHeight * (1 - 2 * padRatio));

    DrawRectangleRounded(
        {listRec.x + listRec.width * padRatio, offsetY + cellHeight * padRatio,
         listRec.width * (1 - 2 * padRatio), cellHeight * (1 - 2 * padRatio)},
        0.15, 0, color);

    DrawText(("ID: " + std::to_string(id)).c_str(),
             listRec.x + 2 * padRatio * listRec.width, offsetY + fontOffset,
             fontSize, WHITE);

    DrawText(("P: " + std::to_string(period)).c_str(),
             listRec.x + (2 * padRatio + 89.f / 480.f) * listRec.width,
             offsetY + fontOffset, fontSize, WHITE);

    DrawText(("D: " + std::to_string(duration)).c_str(),
             listRec.x + (2 * padRatio + 209.f / 480.f) * listRec.width,
             offsetY + fontOffset, fontSize, WHITE);

    remove.area = {
        listRec.x + (2 * padRatio + 321.f / 480.f) * listRec.width,
        offsetY + 3 * padRatio * cellHeight,
        (1 - 6 * padRatio) * cellHeight,
        (1 - 6 * padRatio) * cellHeight,
    };
    remove.draw();
  }
}

Controls::Controls(std::function<void(int)> deleteTask,
                   std::function<void(std::pair<long, long>)> addTask,
                   std::function<void(SchedulingAlgo newAlg)> switchAlg,
                   std::filesystem::path execPath)
    : listArea({mainRec.x, mainRec.y + mainRec.height * 3.f / 16.f,
                mainRec.width * 0.9f, mainRec.height * 5.f / 8.f}),
      edfRec({mainRec.x + mainRec.width / 24.f, mainRec.y + mainRec.height / 40,
              mainRec.width * 5.f / 16.f, mainRec.height * 11.f / 80.f}),
      rmsRec({mainRec.x + mainRec.width * 31.f / 48.f,
              mainRec.y + mainRec.height / 40, mainRec.width * 5.f / 16.f,
              mainRec.height * 11.f / 80.f}),
      deleteTaskInterface(deleteTask), addTaskInterface(addTask),
      switchAlgInterface(switchAlg), execPath(execPath),
      addIcon(LoadTexture((execPath / "assets/add.png").string().c_str())),
      switchIcon(
          LoadTexture((execPath / "assets/switch.png").string().c_str())),
      deleteIcon(
          LoadTexture((execPath / "assets/delete.png").string().c_str())),
      upIcon(LoadTexture((execPath / "assets/up.png").string().c_str())),
      durationIn({mainRec.x, mainRec.y + mainRec.height * 0.8125f,
                  mainRec.width * 3 / 4, mainRec.height * 0.09375f},
                 "Task duration", BLACK),
      periodIn({mainRec.x, mainRec.y + mainRec.height * 0.90625f,
                mainRec.width * 3 / 4, mainRec.height * 0.09375f},
               "Task Period", BLACK),
      bAddTask({mainRec.x + 350 * mainRec.width / 480,
                mainRec.y + mainRec.height * 665 / 800,
                mainRec.width * 12.f / 48.f, mainRec.height * 12.f / 80.f},
               {}, fConfirm, addIcon),
      bSwitchAlg({mainRec.x + mainRec.width * 19 / 48,
                  mainRec.y + 25 * mainRec.height / 800,
                  mainRec.width * 10 / 48, mainRec.height / 8},
                 {}, fSwitch, switchIcon),
      bScrollUp(
          {mainRec.x + mainRec.width * 0.875f,
           mainRec.y + mainRec.height * 0.5f - 0.1f * mainRec.width,
           0.1f * mainRec.width, 0.1f * mainRec.width},
          WHITE, [this]() { this->scroll(true); }, upIcon),
      bScrolldown(
          {mainRec.x + mainRec.width * 0.875f,
           mainRec.y + mainRec.height * 0.5f + 0.1f * mainRec.width,
           0.1f * mainRec.width, 0.1f * mainRec.width},
          WHITE, [this]() { this->scroll(false); }, upIcon) {
  bScrolldown.flip = true;
}

void Controls::draw() {
  DrawRectangleRounded(mainRec, 0.1, 0, bgColor);
  bSwitchAlg.draw();
  bAddTask.draw();
  periodIn.draw();
  durationIn.draw();

  for (int i = 0; i < tasks.size(); i++) {
    tasks[i].draw(i - listInd, listArea);
  }

  DrawRectangleRounded(edfRec, 0.15, 0, WHITE);
  DrawText("EDF", edfRec.x + edfRec.width * 0.15,
           edfRec.y + edfRec.height * 0.25, edfRec.height * 0.5, BLACK);
  DrawRectangleRounded(rmsRec, 0.15, 0, WHITE);
  DrawText("RMS", rmsRec.x + rmsRec.width * 0.15,
           rmsRec.y + rmsRec.height * 0.25, rmsRec.height * 0.5, BLACK);

  bScrollUp.draw();
  bScrolldown.draw();
}

void Controls::scroll(bool up) {
  if (up) {
    listInd = std::max(0, listInd - 1);
  } else {
    listInd = std::min(tasks.size(), static_cast<unsigned long>(listInd + 1));
  }
}