# 🤖 CNC Robot Simulator

A real-time visual simulator for CNC (Computer Numerical Control) robot toolpaths. Load G-Code programs and watch them execute with an interactive 2D visualization of the tool path, current machine state, and command preview.

---

## 🚀 Project Overview

This project implements a **full-featured CNC interpreter and simulator** that parses G-Code programs and executes them in real-time with visual feedback. The simulator includes an interactive viewport, feed rate override controls, and a HUD displaying machine state.

The robot executes commands such as linear motion, arc interpolation, and dwells while continuously drawing its path dynamically on the 2D workspace.

---

## 🎯 Features

* 📄 **G-Code Support**: Parse and execute standard CNC G-Code (G00, G01, G02/G03, G04, F-words)
* ⚙️ **Real-Time Simulation**: Tick-based execution engine with accurate motion simulation
* 🖥 **Interactive 2D Visualization** using SFML:
  - Zoomable and pannable viewport
  - Dynamic grid with major and minor divisions
  - Tool position indicator
  - Real-time toolpath drawing
* 📊 **HUD Display**:
  - Current X, Y position
  - Feed rate with override multiplier
  - Command counter and progress
  - Program selector for multi-program files
  - Distance traveled tracker
  - Execution status (RUNNING / PAUSED / COMPLETE)
* 🎮 **Interactive Controls**:
  - Play/Pause execution
  - Reset to program start
  - Feed rate override (0.1x - 5.0x)
  - Mouse-based viewport navigation
* 👁️ **Command Preview**: Displays next 8 commands in queue for inspection
* 🔄 **Multi-Program Support**: Load and switch between multiple programs in a single file

---

## 🧾 Supported G-Code Commands

The simulator supports the following standard CNC G-Code words:

| Command | Description | Example |
|---------|-------------|---------|
| **G00** | Rapid positioning (no cutting) | `G00 X100 Y50` |
| **G01** | Linear interpolation at feed rate | `G01 X100 Y50 F200` |
| **G02** | Clockwise arc motion | `G02 X100 Y100 I50 J50 F150` |
| **G03** | Counter-clockwise arc motion | `G03 X100 Y100 I50 J50 F150` |
| **G04** | Dwell (pause) | `G04 P1.5` |
| **F** | Feed rate (units per second) | `F200` |
| **X, Y** | Coordinate values | `X100 Y50` |

Example program file:

```
G01 X100 Y0 F150
G01 X100 Y100 F150
G02 X0 Y100 I-50 J0 F150
G01 X0 Y0 F150
G04 P1.0
```

---

## 🎮 Controls

| Input | Action |
|-------|--------|
| **SPACE** | Play / Pause execution |
| **R** | Reset current program to start |
| **HOME** | Re-center viewport and reset zoom |
| **↑ / ↓** | Increase / Decrease feed rate override |
| **Scroll Wheel** | Zoom in / out |
| **Middle Mouse Drag** | Pan viewport |
| **PLAY Button** | Start execution |
| **PAUSE Button** | Pause execution |
| **RESET Button** | Reset to program start |
| **LOAD Button** | Open file dialog to load G-Code file |
| **PROG +** | Switch to next program |
| **PROG -** | Switch to previous program |

---

## 🧱 Project Architecture

### Core Components

**Robot** (`Robot.h`)
- Maintains machine state: position (X, Y), feed rate
- Stores tool path for visualization
- Tracks total distance traveled

**CommandQueue** (`CommandQueue.h`)
- Parses and manages G-Code programs
- Owns all command objects (Linear, Arc, Dwell)
- Drives execution via `tick()` method with delta-time
- Supports multiple programs per file
- Provides command preview and status information

**Command Types**
- **LinearCommand**: G00 (rapid) or G01 (feed) linear motion
- **ArcCommand**: G02/G03 circular/helical arc motion
- **DwellCommand**: G04 time-based pause

### Main Application (`CNC_Robot_SIM.cpp`)
- SFML window management and rendering
- Event handling (keyboard, mouse, buttons)
- UI rendering (HUD, buttons, preview panel)
- Viewport management (pan, zoom, recenter)
- Feed rate override control

---

## 🛠️ Getting Started

### Requirements
- **C++17** or later
- **SFML** (Simple and Fast Multimedia Library) 3.0+
- **Windows** (uses Windows API for file dialogs)
- `arial.ttf` font file in the working directory

### Building

Compile with SFML:
```bash
g++ -std=c++17 CNC_Robot_SIM.cpp Robot.cpp CommandQueue.cpp -o CNC_Simulator -lsfml-graphics -lsfml-window -lsfml-system
```

Or use your preferred build system with SFML linked.

### Running

1. Ensure `arial.ttf` is in the application directory
2. Launch the executable
3. Click **LOAD** to open a G-Code file
4. Click **PLAY** or press **SPACE** to begin execution
5. Use mouse/keyboard controls to navigate and inspect the toolpath

---

## 📊 UI Layout

```
┌─────────────────────────────────────────────────────────────┐
│ X: 0.00   Y: 0.00                    [PLAY] [PAUSE] [RESET] │
│ Feed: 100.00 u/s x1.00              [LOAD] [PROG+] [PROG-] │
│ Cmd: 5 / 12                                                  │
│ Prog: 1 / 1                          ┌──────────────────┐   │
│ Dist: 234.56 u                       │ -- Next --       │   │
│ Now: G01 X100 Y100 F150              │ G01 X100 Y100    │   │
│ State: RUNNING                       │ G02 X50 Y50      │   │
│                                      │ G01 X0 Y0        │   │
│                    [Main Viewport]   │ G04 P1.0         │   │
│                    (with grid)       └──────────────────┘   │
│                                                              │
│                    [Tool Path Viz]                           │
│                    Red Circle = Tool                         │
│                    Green/Red Lines = Y/X Zero               │
└─────────────────────────────────────────────────────────────┘
```

**Top-Left**: HUD with position, feed, progress, and status  
**Below HUD**: Command preview (next 8 commands)  
**Center**: Interactive viewport with grid and toolpath  
**Bottom**: Control buttons  

---

## 💡 Tips & Usage

- **Program Switching**: Use **PROG +** / **PROG -** to preview different programs before execution without reloading the file
- **Visual Scale**: Grid major lines are 100 units apart; minor lines are 25 units for reference
- **Feed Override**: Useful for slow inspection (0.1x) or accelerated playback (5.0x)
- **Precision**: Green line = Y=0 axis; Red line = X=0 axis; Yellow cross = origin
- **Pause & Inspect**: Pause execution and zoom into specific toolpath sections for detailed inspection

---

## 📝 Example G-Code Programs

### Square Pattern
```
G01 X100 Y0 F200
G01 X100 Y100 F200
G01 X0 Y100 F200
G01 X0 Y0 F200
```

### Circle (approx. with arcs)
```
G01 X50 Y0 F150
G02 X50 Y0 I-50 J0 F150
```

### Complex Shape with Dwell
```
G01 X50 Y50 F150
G01 X100 Y100 F150
G04 P2.0
G01 X0 Y0 F150
```

---

## 📄 File Format

G-Code files can be:
- `.txt` - Plain text G-Code
- `.nc` - CNC file format
- `.gcode` - G-Code format

Multiple programs can be stored in a single file, separated appropriately (consult your CommandQueue implementation for details on multi-program file format).

---

## ⚙️ Configuration

Key tunable parameters in `CNC_Robot_SIM.cpp`:

- **Window Resolution**: `W = 1200`, `H = 760`
- **Framerate**: `window.setFramerateLimit(60)`
- **Grid Spacing**: `MAJOR = 100.f`, `MINOR = 25.f`
- **Feed Override Range**: `0.1x` to `5.0x`
- **Tool Marker Size**: `5.f` pixels radius

---

## 📦 Project Structure

```
CNC_Robot_SIM/
├── CNC_Robot_SIM.cpp         # Main application (UI, viewport, event loop)
├── Robot.h / Robot.cpp        # Machine state and toolpath buffer
├── CommandQueue.h / CommandQueue.cpp  # G-Code parser and executor
├── README.md                  # This file
└── arial.ttf                  # Font file (required)
├── Robot.h
├── Command.h
├── MoveCommand.h / MoveCommand.cpp
├── Parser.h
├── commands.txt
```

---

## ⚙️ Technologies Used

* C++
* SFML (Simple and Fast Multimedia Library)
* Object-Oriented Programming (OOP)

---

## 🛠 Setup Instructions

### 1. Install SFML

Download SFML from the official website and extract it.

### 2. Configure Visual Studio

#### Include Directory:

```
C:\SFML\include
```

#### Library Directory:

```
C:\SFML\lib
```

#### Linker Dependencies (Debug):

```
sfml-graphics-d.lib
sfml-window-d.lib
sfml-system-d.lib
```

---

### 3. Copy Required DLLs

From:

```
C:\SFML\bin
```

To your executable directory:

```
sfml-graphics-d-2.dll
sfml-window-d-2.dll
sfml-system-d-2.dll
```

---

### 4. Build & Run

* Set build configuration to **x64**
* Run the project
* The simulator window should open and execute commands

---

## 🧪 How It Works

1. The parser reads instructions from `commands.txt`
2. Commands are converted into executable objects
3. The simulation engine processes commands step-by-step
4. The robot moves and draws its path in real time

---

## 🎓 Educational Value

This project demonstrates:

* Command parsing and interpretation
* Simulation of physical movement
* Real-time graphics rendering
* Basic robotics and CNC concepts

---

## 🔮 Future Improvements

* Add WAIT command
* Support full G-code syntax
* Add GUI controls (Run / Pause / Reset)
* Grid workspace visualization
* Z-axis support (3D simulation)
* Collision detection

---

## 📌 Author

Developed as a semester project for Mechatronics Engineering.

---

## ⭐ Notes

This is a simplified simulation and does not represent a full industrial CNC system, but it demonstrates the core concepts of motion control and instruction execution.

---
