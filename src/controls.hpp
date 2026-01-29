#pragma once
#include "process.hpp"
#include <filesystem>
#include <functional>
#include <optional>
#include <raylib.h>
#include <string>
#include <utility>
#include <vector>

class Button {
  Rectangle area;
  std::function<void(void)> onClick;
  std::optional<Texture> icon;
  Color color;
  bool flip = false;

public:
  Button(Rectangle area, Color, std::function<void(void)> onClick);
  Button(Rectangle area, Color, std::function<void(void)> onClick,
         Texture icon);
  void draw();
  friend class TaskCard;
  friend class Controls;
};

class InputCard {
  bool active = false;
  Color color;
  Rectangle area;
  float padRatio = 0.075;
  std::string defaultText;

public:
  std::string input;
  InputCard(Rectangle area, std::string defaultText, Color color);
  void setActive(bool active);
  void draw();
};

class TaskCard {
  Color color = {44, 48, 59, 255};
  long period;
  long duration;
  std::function<void(int)> fRemove;
  Button remove;

public:
  int id;
  TaskCard(int id, long period, long duration, Color color,
           std::function<void(int)> fRemove, Texture buttonIcon);
  void draw(int index, Rectangle listRec);
};

class Controls {
  Color bgColor = {146, 165, 203, 255};
  Rectangle mainRec = {800, 0, 480, 800};
  Rectangle edfRec;
  Rectangle rmsRec;
  Rectangle listArea;
  int listInd = 0;
  SchedulingAlgo currentAlg = SchedulingAlgo::EDF;
  std::filesystem::path execPath;

  std::function<void(int)> deleteTaskInterface;
  InputCard durationIn;
  InputCard periodIn;

  std::function<void(std::pair<long, long>)> addTaskInterface;
  std::function<void(SchedulingAlgo newAlg)> switchAlgInterface;

  std::function<void(void)> fConfirm = [this]() {
    try {
      long period = std::stol(this->periodIn.input);
      try {
        long duration = std::stol(this->durationIn.input);
        this->addTaskInterface({period, duration});
      } catch (const std::exception &e) {
      }
    } catch (const std::exception &e) {
    }
  };

  std::function<void(void)> fSwitch = [this]() {
    if (currentAlg == SchedulingAlgo::EDF) {
      switchAlgInterface(SchedulingAlgo::RMS);
      currentAlg = SchedulingAlgo::RMS;
      bSwitchAlg.flip = true;
    } else {
      switchAlgInterface(SchedulingAlgo::EDF);
      currentAlg = SchedulingAlgo::EDF;
      bSwitchAlg.flip = false;
    }
  };
  void scroll(bool up);
  Texture addIcon;
  Texture switchIcon;
  Texture upIcon;

  Button bAddTask;
  Button bSwitchAlg;
  Button bScrollUp;
  Button bScrolldown;

public:
  Texture deleteIcon;
  std::vector<TaskCard> tasks;
  Controls(std::function<void(int)> deleteTask,
           std::function<void(std::pair<long, long>)> addTask,
           std::function<void(SchedulingAlgo newAlg)> switchAlg,
           std::filesystem::path execPath);

  void handleInput();
  void draw();
};