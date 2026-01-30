#pragma once
#include "process.hpp"
#include <array>
#include <filesystem>
#include <functional>
#include <optional>
#include <raylib.h>
#include <string>
#include <utility>
#include <vector>

class Button {
  Rectangle area;
  std::optional<Texture> icon;
  Color color;
  bool flip = false;

public:
  Button(Rectangle area, Color, Texture icon);
  void draw();
  friend class TaskCard;
  friend class Controls;
};

class InputCard {
  Color color;
  Rectangle area;
  float padRatio = 0.075;
  std::string defaultText;

public:
  std::string input;
  InputCard(Rectangle area, std::string defaultText, Color color);
  bool active = false;
  void draw();
};

class TaskCard {
  Color color = {44, 48, 59, 255};
  long period;
  long duration;
  Button remove;

public:
  int id;
  TaskCard(int id, long period, long duration, Color color, Texture buttonIcon);
  void draw(int index, Rectangle listRec);
};

class Controls {
  std::array<bool, 5> cardsInView = {false};
  Color bgColor = {146, 165, 203, 255};
  Rectangle mainRec = {800, 0, 480, 800};
  Rectangle listArea;
  Rectangle edfRec;
  Rectangle rmsRec;
  int listInd = 0;
  std::array<Rectangle, 11> clickables;
  SchedulingAlgo currentAlg = SchedulingAlgo::EDF;
  std::filesystem::path execPath;

  std::function<void(int)> deleteTaskInterface;
  InputCard durationIn;
  InputCard periodIn;

  std::function<void(std::pair<long, long>)> addTaskInterface;
  std::function<void(SchedulingAlgo newAlg)> assignAlgInterface;
  void addTask() {
    long period = 0, duration = 0;
    try {
      if (!this->periodIn.input.empty()) {
        period = std::stol(this->periodIn.input);
      }
      this->periodIn.input.clear();
      try {
        if (!this->durationIn.input.empty()) {
          duration = std::stol(this->durationIn.input);
        }
        this->durationIn.input.clear();
        if (period && duration) {
          this->addTaskInterface({period, duration});
        }
      } catch (const std::exception &e) {
      }
    } catch (const std::exception &e) {
    }
  };

  void switchAlg() {
    if (currentAlg == SchedulingAlgo::EDF) {
      assignAlgInterface(SchedulingAlgo::RMS);
      currentAlg = SchedulingAlgo::RMS;
      bSwitchAlg.flip = true;
    } else {
      assignAlgInterface(SchedulingAlgo::EDF);
      currentAlg = SchedulingAlgo::EDF;
      bSwitchAlg.flip = false;
    }
  }
  void scroll(bool up);
  Texture addIcon;
  Texture switchIcon;
  Texture upIcon;

  Button bAddTask;
  Button bSwitchAlg;
  Button bScrollUp;
  Button bScrollDown;
  void handleClick(std::pair<float, float> &&pos);

public:
  void setInView();
  Texture deleteIcon;
  std::vector<TaskCard> cards;
  Controls(std::function<void(int)> deleteTask,
           std::function<void(std::pair<long, long>)> addTask,
           std::function<void(SchedulingAlgo newAlg)> switchAlg,
           std::filesystem::path execPath);

  void handleInput();
  void draw();
};