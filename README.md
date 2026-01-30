
# Real-Time Scheduler Simulation

![C++](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg) ![Build](https://img.shields.io/badge/Build-CMake-green.svg) ![Library](https://img.shields.io/badge/Graphics-Raylib-orange.svg)

A multithreaded visualization of **Earliest Deadline First (EDF)** and **Rate Monotonic Scheduling (RMS)** algorithms.

This project simulates an OS scheduler in real-time, handling task preemption, deadline misses, and context switching events. It features a custom-built GUI and a decoupled architecture where the simulation logic runs on a dedicated thread, synchronized with the visualization layer via a thread-safe event queue.

![Simulation Screenshot](working.png)

##  Key Features

*   **Real-Time Algorithms:** Toggle between **EDF** (Dynamic Priority) and **RMS** (Static Priority) on the fly.
*   **Dynamic Task Management:** Create, edit, and delete tasks dynamically while the simulation is running.
*   **Visual Gantt Chart:** Live timeline rendering of task execution, preemptions, and deadline misses.
*   **Process Tray:** Visualizes the state of all active processes (Ready, Waiting, Running).
*   **Custom UI System:** A lightweight, immediate-mode style UI built from scratch using Raylib primitives (no external UI libraries used).

##  Technical Highlights

This project demonstrates core **Systems Programming** concepts:

*   **Modern C++20:** extensive use of `std::jthread`, `std::ranges`, `std::views`, and structured bindings.
*   **Concurrency & Synchronization:**
    *   Implements a **Producer-Consumer** pattern between the Scheduler (Producer) and the UI (Consumer).
    *   Uses `std::condition_variable` and `std::mutex` for precise thread sleeping and waking (no busy-wait loops).
    *   Handles "interrupts" (Task Added/Edited) safely within the scheduling loop using `wait_until`.
*   **Decoupled Architecture:** The `Scheduler` class is completely agnostic of the rendering engine. It communicates purely via an event interface, adhering to the Single Responsibility Principle.
*   **Resource constrained design:** Efficient memory management and minimized lock contention.

##  Architecture

The application is split into two main execution threads:

1.  **Main Thread (UI & Input):**
    *   Handles Raylib rendering (`View` class).
    *   Processes User Input (`Controls` class).
    *   Consumes events from the thread-safe queue to update the visual state.
2.  **Scheduler Thread (Logic):**
    *   Manages the ready queue and priority logic.
    *   Simulates clock ticks and context switches.
    *   Pushes events (`Start`, `Preempt`, `Miss`, `Complete`) to the UI.

## Build Instructions

### Prerequisites
*   **C++ Compiler:** GCC 10+, Clang 10+, or MSVC (Must support C++20).
*   **CMake:** Version 3.11 or higher.
*   **Raylib:** The project requires Raylib installed on your system.

### Compiling

```bash
# 1. Clone the repository
git clone https://github.com/Hatem-707/edf-simulation.git
cd edf-simulation

# 2. Create build directory
mkdir build && cd build

# 3. Configure and Build
cmake ..
make

# 4. Run
./main
```

##  Usage

1.  **Add Task:** Enter a **Period** and **Duration** (in milliseconds) in the input fields and click `+`.
2.  **Switch Algorithm:** Click the `Switch Algorithm` button to toggle between EDF and RMS.
3.  **Delete Task:** Click the `X` icon on any active task card in the list.
4.  **Observe:**
    *   **Timeline:** Shows the history of execution.
    *   **Logs:** Icons indicate successful completion, preemption, or missed deadlines.

##  Project Structure

```
├── src/
│   ├── app.cpp         # Bridge between View and Scheduler
│   ├── controls.cpp    # Custom UI implementation (Inputs, Buttons, Cards)
│   ├── main.cpp        # Entry point
│   ├── process.hpp     # Enums and Event definitions
│   ├── scheduler.cpp   # Core scheduling logic (Threaded)
│   └── view.cpp        # Visualization logic (Raylib drawing)
├── CMakeLists.txt      # Build configuration
└── README.md
```


##  License

This project is open-source and available under the MIT License.