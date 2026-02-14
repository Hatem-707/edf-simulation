

# Real-Time Scheduler Simulation

![C++](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg) ![Build](https://img.shields.io/badge/Build-CMake-green.svg) ![Library](https://img.shields.io/badge/Graphics-Raylib-orange.svg)

This is a multithreaded RTOS scheduling simulation implementing event queues, condition variables, and immediate GUI rendering. It demonstrates the practical application of modern C++, structured bindings, ranges, and jthread. Throughout the project, I aimed to combine my OS course concepts (scheduling and synchronization) with modern C++ standards.

The app logic is split into two main threads: **Rendering** and **Scheduling**. This decoupled architecture allowed me to use **Event-Driven Waiting** on the scheduler for efficiency without compromising the rendering clock.

Scheduling is done via `std::chrono` timers and condition variables where the scheduler waits for an event to run its logic: pick a new runner, push events to the GUI, and blocking-wait for the next event. The rendering thread displays a dynamic Gantt chart and can handles task insertion/deletion, which wakes up the scheduling thread.

## Demo
![Simulation Screenshot 1](demo_1.png)
![Simulation Screenshot 2](demo_2.png)
<video src="https://github.com/user-attachments/assets/bb4b391a-c8ad-4c73-be63-5d7508084574" controls="controls" style="max-width:100%;"> </video>

## Features
1. **Dynamic Task Editing** \
    On-the-fly task insertion and deletion.
2. **Scheduling Algorithm Change** \
    Allows switching between Earliest Deadline First (EDF) and Rate Monotonic Scheduling (RMS).
3. **Condition Variable Synchronization** \
    Efficient sleeping achieved through condition variables and `wait_until`.
4. **Producer-Consumer Multithreading** \
    The two threads use non-blocking queues to stage and handle changes safely.

## Modern C++ Primitives
1. **Ranges** \
    Cleaner, safer, and more efficient looping.
2. **Structured Bindings** \
    More readable tuple handling.
3. **Smart Pointers** \
    RAII and automatic memory management.
4. **jthread** \
    Automatic thread joining (C++20).
5. **std::function and Lambdas** \
    Method capturing and calling within object attributes.

## Architecture
1. **Views (view.cpp)** \
    Consumes scheduler events and renders the Gantt chart and waiting tasks tray.
2. **Controls (controls.cpp)** \
    Renders the UI section and handles input events, passing them to the scheduler.
3. **Scheduler (scheduler.cpp)** \
    Owns the task queue and produces events for the Views.
4. **Application Controller (app.cpp)** \
    Acts as the composition root to combine components and bind interfaces for communication.
5. **main.cpp** \
    The entry point.

## Compiling
The project assumes you have Raylib (v5.5) on the system and requires a C++20 compiler. If you don't have Raylib installed, uncomment the `FetchContent` lines in `CMakeLists.txt`.

```bash
mkdir build && cd build
cmake ..
make
./bin/main
```

