# 🤖 CNC Robot Simulator

A real-time 2D visual simulator for CNC toolpaths. Load G-code programs and
watch them execute with a live, fading toolpath trail, an interactive
viewport, and a sidebar HUD showing machine state.

---

## 🚀 Project Overview

This project implements a **command-pattern CNC interpreter and simulator**
that parses G-code programs and executes them in real time with visual
feedback. Each motion command (linear, arc, dwell) is its own self-contained
object that owns its execution state — no global flags, no raw pointers.

The robot executes its program tick-by-tick using delta time, so motion is
frame-rate independent, and the resulting toolpath is drawn live with a
glowing "active cut" trail that fades to a permanent grey line over time.

---

## 🎯 Features

* 📄 **G-code support**: G00, G01, G02/G03, G04, F-words, I/J arc offsets
* 🔁 **Modal G-code carry-over**: a line with only `X Y` reuses the last motion type
* 💬 **Inline comments** with `;`
* ✏️ **Shorthand support**: `G0/G1/G2/G3` alongside `G00/G01/G02/G03`
* ⚙️ **Real-time tick-based execution** — frame-rate independent
* 🖥 **Interactive 2D viewport**:
  - Zoomable and pannable (scroll + middle-drag)
  - Major/minor grid with coloured X/Y axes
  - Crosshair-and-ring tool marker
  - **Fading toolpath trail** — recent cuts glow white, fade to dim grey over ~3.5s
* 📊 **Sidebar HUD**:
  - Live X, Y position
  - Raw feed rate, actual (override-scaled) feed rate, and override %
  - Command and program progress counters
  - Distance travelled
  - Execution status (RUNNING / PAUSED / COMPLETE)
  - Upcoming command queue preview
* 🎮 **Controls**: Play/Pause, Reset, Load, feed override (0.1x–5.0x), program switching
* 🔄 **Multi-program files** — separate programs with `NEW`, cycle with PROG +/-

---

## 🧾 Supported G-Code Commands

| Command | Description | Example |
|---------|-------------|---------|
| **G00 / G0** | Rapid positioning (no cutting, no trail) | `G00 X100 Y50` |
| **G01 / G1** | Linear interpolation at feed rate | `G01 X100 Y50 F200` |
| **G02 / G2** | Clockwise arc motion | `G02 X100 Y100 I50 J50 F150` |
| **G03 / G3** | Counter-clockwise arc motion | `G03 X100 Y100 I50 J50 F150` |
| **G04 / G4** | Dwell (pause) | `G04 P1.5` |
| **F**  | Feed rate, units/second (persists until changed) | `F200` |
| **X, Y** | Target coordinates (absolute) | `X100 Y50` |
| **I, J** | Arc centre offset from current position (G02/G03 only) | `I-50 J0` |
| `;`    | Inline comment — rest of line ignored | `G01 X0 Y0 ; return home` |
| `NEW`  | Start a new program in the same file | |

A full circle is created by setting the arc's target equal to its start
point — the simulator detects this and sweeps the full 360°.

---

## 🎮 Controls

| Input | Action |
|-------|--------|
| **SPACE** | Play / Pause execution |
| **R** | Reset current program to start |
| **HOME** | Re-centre viewport and reset zoom |
| **↑ / ↓** | Increase / decrease feed rate override |
| **Scroll Wheel** | Zoom in / out (over canvas only) |
| **Middle Mouse Drag** | Pan viewport (over canvas only) |
| **PLAY / PAUSE** | Start / stop execution |
| **RESET** | Reset position, clear path, return to start |
| **LOAD** | Open file dialog to load a G-code file |
| **PROG + / PROG -** | Switch between programs in the loaded file |

---

## 🧱 Project Architecture

```
Robot          — machine state: position, feed rate, path buffers, colour palette
Command        — abstract base for all motion types (execute, isFinished, label)
├── LinearCommand   — G00 (rapid, no trail) / G01 (cutting)
├── ArcCommand      — G02 / G03, owns all arc-interpolation state internally
└── DwellCommand    — G04, pure time delay
CommandQueue   — parses files, owns all programs, drives execution via tick()
main.cpp       — window, input, sidebar UI, rendering
```

### Robot (`Robot.h`)
- Holds position, raw feed rate, and execution stats (distance travelled,
  commands executed)
- Owns the toolpath colour palette (`Palette` namespace) — every colour in
  the app is defined in one place
- Manages the two-layer path system (see below)

### CommandQueue (`CommandQueue.h`)
- Parses G-code files into `std::unique_ptr<Command>` objects — no raw
  `new`/`delete` anywhere
- Supports modal carry-over, shorthand G-words, comments, and `NEW`
  program separators
- Drives execution one tick at a time and exposes program/command
  navigation and a preview of upcoming commands

### Command types
- **LinearCommand** — G00 moves silently (no path geometry). G01 calls
  `robot.addSegment()` each frame it's cutting.
- **ArcCommand** — all arc state (start angle, total sweep, radius, centre,
  previous position) lives as private members, initialised on first
  `execute()` call. Handles full-circle arcs and snaps to the exact
  endpoint on completion.
- **DwellCommand** — advances an internal timer; `isFinished()` once the
  dwell duration elapses.

### Toolpath rendering — two-layer fading trail

- **`permanentPath`** — static, dim grey, everything older than the trail
  window. Rendered once, never rebuilt.
- **`trail`** — a rolling deque of recent segments. Each frame, segments
  are aged, faded from bright white to dim grey using an ease-in curve,
  and flushed into `permanentPath` once they exceed `TRAIL_DURATION` (3.5s).

This keeps the "live cut glow" smooth and cheap regardless of total path
length.

### Feed override isolation

`robot.feed` always holds the **raw G-code feed rate**. The override
multiplier is applied only as a temporary scale on motion speed during
each tick (`robot.feed *= overrideF` → tick → `robot.feed /= overrideF`),
so changing playback speed never changes the recorded path geometry.

---

## 🛠️ Getting Started

### Requirements
- **C++20** or later (uses `std::optional`, SFML 3's `pollEvent`/`getIf`
  event API, and `sf::Vector2f` arithmetic operators)
- **SFML 3.0+**
- **Windows** (uses `<windows.h>` / `<commdlg.h>` for the LOAD file dialog)
- `arial.ttf` font file in the working directory
- `command.txt` in the working directory (auto-loaded on startup)

### Building

```bash
g++ -std=c++20 main.cpp -lsfml-graphics -lsfml-window -lsfml-system -o cnc_sim
```

Or configure your build system / IDE with SFML 3 linked as below.

### Setting up SFML 3 in Visual Studio

**Include directory:**
```
C:\SFML\include
```

**Library directory:**
```
C:\SFML\lib
```

**Linker dependencies (Debug, SFML 3):**
```
sfml-graphics-d.lib
sfml-window-d.lib
sfml-system-d.lib
```

**Required DLLs** (copy from `C:\SFML\bin` to your executable directory):
```
sfml-graphics-d-3.dll
sfml-window-d-3.dll
sfml-system-d-3.dll
```

Set the build configuration to **x64**.

### Running

1. Ensure `arial.ttf` and `command.txt` are in the application directory
2. Launch the executable — `command.txt` loads automatically
3. Click **LOAD** to open a different G-code file
4. Click **PLAY** or press **SPACE** to begin execution
5. Use mouse/keyboard to navigate and inspect the toolpath

---

## 📦 Project Structure

```
cnc-simulator/
├── main.cpp           # Window, input, sidebar UI, rendering
├── Robot.h            # Machine state, colour palette, path/trail logic
├── Command.h          # Abstract command base + shared math helpers
├── LinearCommand.h     # G00 / G01
├── ArcCommand.h        # G02 / G03
├── DwellCommand.h      # G04
├── CommandQueue.h      # File parsing, program management, execution
├── command.txt         # Demo G-code (6 programs)
├── arial.ttf            # Required font
└── README.md
```

---

## 💡 Tips & Usage

- **Program switching**: use PROG +/- to preview different programs
  without reloading the file
- **Visual scale**: major grid lines are 100 units apart, minor lines 25 units
- **Feed override**: slow down to 0.1x for detailed inspection or speed up
  to 5.0x for a fast preview — this never changes the recorded toolpath
- **Axes**: red line = X axis (Y=0), green line = Y axis (X=0), yellow
  cross = origin
- **Pause & inspect**: pause execution and zoom into specific sections of
  the toolpath for detail

---

## 📝 Example G-Code Programs

### Square pattern
```
G01 X100 Y0 F200
G01 X100 Y100 F200
G01 X0 Y100 F200
G01 X0 Y0 F200
```

### Circle via arc
```
G01 X50 Y0 F150
G02 X50 Y0 I-50 J0 F150
```

### Complex shape with dwell
```
G01 X50 Y50 F150
G01 X100 Y100 F150
G04 P2.0
G01 X0 Y0 F150
```

### Multi-program file
```
G01 X100 Y0 F150
G01 X0 Y0 F150
NEW
G02 X50 Y0 I-25 J0 F100
NEW
G04 P1.0
G01 X50 Y50 F200
```

---

## 📄 File Formats Accepted by LOAD

- `.txt` — plain text G-code
- `.nc` — CNC file format
- `.gcode` — G-code format

---

## ⚙️ Configuration

Key tunable parameters in `main.cpp` / `Robot.h`:

- **Window resolution**: `W = 1280`, `H = 800`
- **Framerate**: `window.setFramerateLimit(60)`
- **Sidebar width**: `Sidebar::W = 240`
- **Grid spacing**: `MAJOR = 100.f`, `MINOR = 25.f`
- **Feed override range**: `0.1x` to `5.0x`
- **Trail fade duration**: `Robot::TRAIL_DURATION = 3.5f` seconds
- **Tool marker**: 5px ring + 9px crosshair
- **Colour palette**: `Palette` namespace in `Robot.h`

---

## 🧪 How It Works

1. `CommandQueue::loadFile()` reads `command.txt` line by line, applying
   modal carry-over and comment stripping, and builds one or more programs
   of `unique_ptr<Command>`
2. Each frame, `CommandQueue::tick()` calls `execute(robot, dt)` on the
   current command
3. Cutting commands (G01/G02/G03) call `robot.addSegment()`, which pushes
   into the fading trail
4. `Robot::updateTrail(dt)` ages trail segments, fades their colour, and
   flushes expired ones into the permanent path
5. `main.cpp` renders the grid, permanent path, fading trail, tool marker,
   and sidebar HUD every frame

---

## 🎓 Educational Value

This project demonstrates:

* The command pattern for modeling discrete, stateful operations
* Frame-rate-independent simulation via delta time
* Real-time 2D graphics rendering with SFML 3
* G-code parsing and modal state handling
* Basic CNC motion concepts: rapid/linear/arc interpolation, feed rate, dwell

---

## 🔮 Future Improvements

* Z-axis support (true 3D toolpaths)
* Collision detection
* Full G-code syntax (canned cycles, tool offsets, units G20/G21)
* Save/export rendered toolpath as an image
* Spindle on/off (M03/M05) visualization

---

## 📌 Author

Developed as a Mechatronics Engineering project — Egyptian Chinese University.

---

## ⭐ Notes

This is a simplified simulation and does not represent a full industrial
CNC system, but it demonstrates the core concepts of motion control,
G-code interpretation, and real-time instruction execution.
