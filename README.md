# 🤖 CNC-Style Robot Simulator (C++ / SFML)

A simple yet powerful simulation of a CNC-style robot that executes movement instructions from a custom command language and visualizes them in real time.

---

## 🚀 Project Overview

This project implements a **mini CNC interpreter** that reads instructions from a file and simulates robot motion on a 2D workspace.

The robot follows commands such as movement along axes and draws its path dynamically, similar to how CNC machines execute toolpaths.

---

## 🎯 Features

* 📄 Command-based instruction system
* ⚙️ Real-time simulation engine
* 🖥 2D visualization using SFML
* ✏️ Path drawing (toolpath simulation)
* 🧠 Basic command parsing

---

## 🧾 Supported Commands

Example command file:

```
MOVE X 300
MOVE Y 200
MOVE X 500
MOVE Y 400
```

### Command Description

| Command      | Description              |
| ------------ | ------------------------ |
| MOVE X value | Move robot to X position |
| MOVE Y value | Move robot to Y position |

---

## 🧱 Project Structure

```
CNCSimulator/
│
├── main.cpp
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
