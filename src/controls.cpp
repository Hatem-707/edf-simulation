#include "controls.hpp"
#include "process.hpp"
#include <algorithm>
#include <filesystem>
#include <raylib.h>
#include <string>

Button::Button(Rectangle area, Color color, Texture icon)
    : area(area), color(color), icon(icon) {}

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
  if (active) {
    DrawRectangleRoundedLinesEx(
        {area.x + area.width * padRatio, area.y + area.height * padRatio,
         area.width * (1.f - 2 * padRatio), area.height * (1.f - 2 * padRatio)},
        0.25, 0, 3, RED);
  }
  DrawRectangleRounded(
      {area.x + area.width * padRatio, area.y + area.height * padRatio,
       area.width * (1.f - 2 * padRatio), area.height * (1.f - 2 * padRatio)},
      0.25, 0, color);
  DrawText((input.empty()) ? defaultText.c_str() : input.c_str(),
           area.x + area.width * 2 * padRatio,
           area.y + area.height * 0.6 * (1.f - 2 * padRatio),
           static_cast<int>(area.height * 0.4 * (1.f - 2 * padRatio)), WHITE);
}

TaskCard::TaskCard(int id, long period, long duration, Color color,
                   Texture buttonIcon)
    : remove({}, BLACK, buttonIcon), id(id), duration(duration), period(period),
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
      assignAlgInterface(switchAlg), execPath(execPath),
      addIcon(LoadTexture((execPath / "assets/add.png").string().c_str())),
      switchIcon(
          LoadTexture((execPath / "assets/switch.png").string().c_str())),
      deleteIcon(
          LoadTexture((execPath / "assets/delete.png").string().c_str())),
      upIcon(LoadTexture((execPath / "assets/up.png").string().c_str())),
      clickables({
          {mainRec.x + mainRec.width * 19 / 48,
           mainRec.y + 25 * mainRec.height / 800, mainRec.width * 10 / 48,
           mainRec.height / 8},
          {mainRec.x + mainRec.width * 0.875f,
           mainRec.y + mainRec.height * 0.5f - 0.1f * mainRec.width,
           0.1f * mainRec.width, 0.1f * mainRec.width},
          {mainRec.x + mainRec.width * 0.875f,
           mainRec.y + mainRec.height * 0.5f + 0.1f * mainRec.width,
           0.1f * mainRec.width, 0.1f * mainRec.width},
          {mainRec.x + 350 * mainRec.width / 480,
           mainRec.y + mainRec.height * 665 / 800, mainRec.width * 12.f / 48.f,
           mainRec.height * 12.f / 80.f},
          {mainRec.x, mainRec.y + mainRec.height * 0.8125f,
           mainRec.width * 3 / 4, mainRec.height * 0.09375f},
          {mainRec.x, mainRec.y + mainRec.height * 0.90625f,
           mainRec.width * 3 / 4, mainRec.height * 0.09375f},
          {
              listArea.x + (2 * 0.05f + 321.f / 480.f) * listArea.width,
              listArea.y + 3 * 0.05f * listArea.height / 5.f,
              (1 - 6 * 0.05f) * listArea.height / 5.f,
              (1 - 6 * 0.05f) * listArea.height / 5.f,
          },
          {
              listArea.x + (2 * 0.05f + 321.f / 480.f) * listArea.width,
              listArea.y + 3 * 0.05f * listArea.height / 5.f +
                  listArea.height / 5.f,
              (1 - 6 * 0.05f) * listArea.height / 5.f,
              (1 - 6 * 0.05f) * listArea.height / 5.f,
          },
          {
              listArea.x + (2 * 0.05f + 321.f / 480.f) * listArea.width,
              listArea.y + 3 * 0.05f * listArea.height / 5.f +
                  listArea.height * 2 / 5.f,
              (1 - 6 * 0.05f) * listArea.height / 5.f,
              (1 - 6 * 0.05f) * listArea.height / 5.f,
          },
          {
              listArea.x + (2 * 0.05f + 321.f / 480.f) * listArea.width,
              listArea.y + 3 * 0.05f * listArea.height / 5.f +
                  listArea.height * 3 / 5.f,
              (1 - 6 * 0.05f) * listArea.height / 5.f,
              (1 - 6 * 0.05f) * listArea.height / 5.f,
          },
          {
              listArea.x + (2 * 0.05f + 321.f / 480.f) * listArea.width,
              listArea.y + 3 * 0.05f * listArea.height / 5.f +
                  listArea.height * 4 / 5.f,
              (1 - 6 * 0.05f) * listArea.height / 5.f,
              (1 - 6 * 0.05f) * listArea.height / 5.f,
          },
      }),
      durationIn(clickables[4], "Task duration", BLACK),
      periodIn(clickables[5], "Task Period", BLACK),
      bAddTask(clickables[3], {}, addIcon),
      bSwitchAlg(clickables[0], {}, switchIcon),
      bScrollUp(clickables[1], WHITE, upIcon),
      bScrollDown(clickables[2], WHITE, upIcon) {
  bScrollDown.flip = true;
}

void Controls::draw() {
  DrawRectangleRounded(mainRec, 0.1, 0, bgColor);
  DrawRectangleRounded(edfRec, 0.15, 0, WHITE);
  DrawText("EDF", edfRec.x + edfRec.width * 0.15,
           edfRec.y + edfRec.height * 0.25, edfRec.height * 0.5, BLACK);
  DrawRectangleRounded(rmsRec, 0.15, 0, WHITE);
  DrawText("RMS", rmsRec.x + rmsRec.width * 0.15,
           rmsRec.y + rmsRec.height * 0.25, rmsRec.height * 0.5, BLACK);
  bSwitchAlg.draw();
  bAddTask.draw();
  periodIn.draw();
  durationIn.draw();
  bScrollUp.draw();
  bScrollDown.draw();

  for (int i = 0; i < cards.size(); i++) {
    cards[i].draw(i - listInd, listArea);
  }
}

void Controls::setInView() {
  listInd = std::min(listInd, static_cast<int>(cards.size()));
  int i = listInd;
  for (; i < cards.size() && i < 5; i++) {
    cardsInView[i - listInd] = true;
  }
  for (; i < 5; i++) {
    cardsInView[i - listInd] = false;
  }
}

void Controls::scroll(bool up) {
  if (up) {
    listInd = std::max(0, listInd - 1);
  } else {
    listInd =
        std::min(cards.size() - 1, static_cast<unsigned long>(listInd + 1));
  }
}

void Controls::handleClick(std::pair<float, float> &&pos) {
  const auto &[x, y] = pos;
  int index = -1;
  for (int i = 0; i < clickables.size(); i++) {
    const Rectangle &currentRec = clickables[i];
    if (x >= currentRec.x && x <= currentRec.x + currentRec.width &&
        y >= currentRec.y && y <= currentRec.y + currentRec.height) {
      index = i;
      break;
    }
  }
  switch (index) {
  case 0:
    switchAlg();
    break;
  case 1:
    scroll(false);
    break;
  case 2:
    scroll(true);
    break;
  case 3:
    addTask();
    break;
  case 4:
    durationIn.active = true;
    periodIn.active = false;
    break;
  case 5:
    durationIn.active = false;
    periodIn.active = true;
    break;
  case 6:
  case 7:
  case 8:
  case 9:
  case 10:
    if (cardsInView[index - 6]) {
      deleteTaskInterface(cards[index - 6 + listInd].id);
    }
    break;
  default:
    durationIn.active = false;
    periodIn.active = false;
    break;
  }
}

void Controls::handleInput() {
  // TODO :: the final function
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    Vector2 mousePos = GetMousePosition();
    handleClick({mousePos.x, mousePos.y});
  }
  if (IsKeyPressed(KEY_BACKSPACE)) {
    if (periodIn.active) {
      if (!periodIn.input.empty()) {
        periodIn.input.pop_back();
      }
    } else if (durationIn.active) {
      if (!durationIn.input.empty()) {
        durationIn.input.pop_back();
      }
    }
  }
  if (IsKeyPressed(KEY_ENTER)) {
    addTask();
  }
  int key = GetCharPressed();
  while (key) {
    if (key > 0x2F && key < 0x3A) {
      if (periodIn.active) {
        periodIn.input += std::to_string(key - 0x30);
      } else if (durationIn.active) {
        durationIn.input += std::to_string(key - 0x30);
      }
    }
    key = GetCharPressed();
  }
}