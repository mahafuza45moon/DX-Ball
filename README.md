# Brick Breaker

A DX-Ball / Brick-Breaker game written in **C++17** with **FreeGLUT** and **OpenGL**.  
Clear all three levels as fast as you can — the ball speeds up over time and
falling perk items can help or hurt you.

**Student:** [Your Name]  
**Roll No.:** [Your Roll Number]  
**Course:** [Your Course]

---

## Features

- **3 levels** of increasing brick density (Normal + Strong bricks).
- **Main menu** — Start, Help, Exit. Navigate with arrow keys + Enter or mouse click.
- **Help screen** — full controls and perk legend in-game.
- **Pause / Resume** at any time; return to menu without restarting.
- **Live HUD** — Score, Lives, Time (`mm:ss`), Level, and current ball speed.
- **Ball speed ramp** — base speed grows 6 u/s per second, capped at 600 u/s.
- **Falling perk drops** — 25 % chance to spawn when any brick is destroyed:

  | Icon | Name | Effect | Duration |
  |------|------|--------|----------|
  | **+** (green) | Extra Life | +1 life immediately | instant |
  | **W** (cyan) | Wide Paddle | Paddle → 150 px wide | 10 s |
  | **F** (magenta) | Fast Ball | Ball speed × 1.4 | 10 s |
  | **S** (light-blue) | Slow Ball | Ball speed × 0.7 | 10 s |
  | **X** (red) | Shrink Paddle | Paddle → 60 px wide *(bad!)* | 10 s |

- **Best-time tracking** — fastest run shown on the menu (in-memory).

---

## Dependencies

| Dependency    | Version    | Notes                              |
|---------------|------------|------------------------------------|
| CMake         | ≥ 3.20     | Build system                       |
| FreeGLUT      | 3.4.0      | Windowing + input                  |
| OpenGL / GLU  | system     | Pre-installed on Windows           |
| Visual Studio | 2022       | Primary build target (MSVC)        |
| MinGW / g++   | MSYS2      | Alternative build target           |

---

## How to Build

### Quickest option — `build.bat` (auto-detects toolchain)

Double-click `build.bat` in Explorer, or from a Command Prompt / PowerShell
at the repo root:

```
build.bat          # configure + build
build.bat run      # configure + build + launch the game
build.bat clean    # delete the build\ folder
```

`build.bat` tries two toolchains in order:

1. **Visual Studio 2022 + vcpkg** — if the `VCPKG_ROOT` environment variable
   points to a vcpkg checkout that has `freeglut:x64-windows` installed.
2. **MinGW (MSYS2)** — if `C:\msys64\mingw64\bin\g++.exe` exists and FreeGLUT
   has been built into `%USERPROFILE%\freeglut-install`.

---

### Option A — Visual Studio 2022 + vcpkg (recommended for submission)

1. Install [vcpkg](https://vcpkg.io):
   ```
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   ```

2. Install FreeGLUT:
   ```
   C:\vcpkg\vcpkg install freeglut:x64-windows
   ```

3. Set the environment variable once (or pass it on every cmake call):
   ```
   set VCPKG_ROOT=C:\vcpkg
   ```

4. Configure and build:
   ```
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Release
   ```

5. Run:
   ```
   build\Release\BrickBreaker.exe
   ```

**Tip:** Open the repo folder in VS2022 ("File → Open → Folder"), point CMake
settings to the vcpkg toolchain file, then press **Ctrl+Shift+B**.

---

### Option B — MinGW / MSYS2

1. Install [MSYS2](https://www.msys2.org) to `C:\msys64`.

2. Build FreeGLUT from source (one-time, run in a normal Command Prompt):
   ```
   curl -L -o freeglut-src.tar.gz https://github.com/freeglut/freeglut/releases/download/v3.4.0/freeglut-3.4.0.tar.gz
   "C:\Program Files\7-Zip\7z.exe" x freeglut-src.tar.gz -oC:\Users\%USERNAME%\
   "C:\Program Files\7-Zip\7z.exe" x C:\Users\%USERNAME%\freeglut-src.tar -oC:\Users\%USERNAME%\
   set PATH=C:\msys64\mingw64\bin;%PATH%
   cmake -S C:\Users\%USERNAME%\freeglut-3.4.0 -B C:\Users\%USERNAME%\freeglut-build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DFREEGLUT_REPLACE_GLUT=ON -DFREEGLUT_BUILD_DEMOS=OFF -DCMAKE_INSTALL_PREFIX=C:\Users\%USERNAME%\freeglut-install
   cmake --build C:\Users\%USERNAME%\freeglut-build
   cmake --install C:\Users\%USERNAME%\freeglut-build
   ```

3. Build the game:
   ```
   set PATH=C:\msys64\mingw64\bin;%PATH%
   cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DFreeGLUT_DIR=C:/Users/%USERNAME%/freeglut-install/lib/cmake/FreeGLUT
   cmake --build build
   build\BrickBreaker.exe
   ```

---

## How to Play

| Input | Action |
|-------|--------|
| **Left / Right arrow keys** | Move paddle |
| **Mouse movement** | Paddle follows cursor X position |
| **Space** or **Left click** | Launch ball |
| **P** | Pause / unpause |
| **M** | Return to main menu (from pause, game over, or win screen) |
| **R** | Restart the run from Level 1 |
| **Esc** | Quit |

- Destroy all bricks to advance to the next level.  
- Survive all **3 levels** to win — your time is recorded as your best.
- Catch falling perk drops with your paddle (see the Feature table above).
- **Blue bricks** — 1 hit, 10 pts  
- **Red bricks** — 2 hits, 20 pts (turn orange after the first hit)

---

## Screenshots

*(Add 1–2 screenshots here after building)*

---

## Known Limitations

- No sound or music (intentional — out of scope for this project).
- Fixed 800 × 600 window.
- Best time is not saved to disk — it resets when the game is closed.
