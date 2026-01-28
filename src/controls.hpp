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
  float padRatio = 0.10;

public:
  Button(Rectangle area, Color, std::function<void(void)> onClick);
  Button(Rectangle area, Color, std::function<void(void)> onClick,
         std::filesystem::path iconPath);
  void draw();
};

class InputCard {
  bool active = false;
  Color color;
  Rectangle area;
  float padRatio = 0.10;
  std::string defaultText;

public:
  std::string input;
  InputCard(Rectangle area, std::string defaultText, Color color);
  void draw();
};

class TaskCard {
  Color color;
  int index;
  int id;
  long period;
  long duration;
  Button remove;

public:
  TaskCard();
  void draw();
};

class Controls {
  Color bgColor = {146, 165, 203, 255};
  Rectangle mainRec = {800, 0, 480, 800};
  int listInd = 0;
  Color darkButton = {44, 48, 59, 255};
  SchedulingAlgo currentAlg = SchedulingAlgo::EDF;
  void shiftList(bool up);

  std::function<void(int)> deleteTaskInterface;
  InputCard durationIn;
  InputCard periodIn;
  std::function<void(std::pair<long, long>)> addTaskInterface;
  Button bAddTask;
  std::function<void(SchedulingAlgo newAlg)> switchAlgInterface;
  Button bSwitchAlg;

public:
  std::vector<TaskCard> tasks;
  Controls(std::function<void(int)> deleteTask,
           std::function<void(std::pair<long, long>)> addTask,
           std::function<void(SchedulingAlgo newAlg)> switchAlg,
           std::filesystem::path pExecutable);
  void draw();
};