# Code_visualiser

## Installation

Three steps on a clean Windows 10/11 machine: install the tools, build Ogre 14 once, then build the visualiser.

### Step 1: Install the tools

| Tool | Notes | Download |
| --- | --- | --- |
| Visual Studio Community (2022 or 2026) | Tick "Desktop development with C++" | [visualstudio.microsoft.com/downloads](https://visualstudio.microsoft.com/downloads/) |
| CMake | Tick "Add CMake to the PATH". Visual Studio 2026 needs CMake 4.2 or newer | [cmake.org/download](https://cmake.org/download/) |
| Git | Defaults are fine | [git-scm.com/downloads](https://git-scm.com/downloads) |
| Python 3.8+ (optional) | Only for visualising your own programs, not the built-in demo. Add to PATH | [python.org/downloads](https://www.python.org/downloads/) |
| LLVM/Clang (optional) | Only for visualising your own programs, not the built-in demo. Add to PATH | [github.com/llvm/llvm-project/releases](https://github.com/llvm/llvm-project/releases) |

### Step 2: Build Ogre 14 (once, about 30 minutes)

Run the commands one at a time and wait for each to finish.

**2.1 Download Ogre**

```
mkdir C:\Ogre
cd C:\Ogre
git clone https://github.com/OGRECave/ogre.git
cd ogre
git checkout v14.6.3
```

**2.2 Configure.** This needs an internet connection, because Ogre downloads its own dependencies.

```
cmake -S . -B C:/Ogre/out/build -DOGRE_BUILD_SAMPLES=OFF -DOGRE_BUILD_TESTS=OFF -DOGRE_BUILD_RENDERSYSTEM_D3D9=OFF
```

**2.3 Build** (the long step)

```
cmake --build C:/Ogre/out/build --config Release
```

**2.4 Install** (creates `C:\Ogre\out\build\sdk`, which the visualiser uses)

```
cmake --build C:/Ogre/out/build --config Release --target install
```

The flag `-DOGRE_BUILD_RENDERSYSTEM_D3D9=OFF` skips Ogre's old DirectX 9 renderer. Code_visualiser draws with OpenGL, and a machine without the old DirectX SDK fails to build DirectX 9 (error `Cannot open include file: 'd3dx9.h'`). Leaving it off the command is only safe if you have that SDK. The [official build notes](https://github.com/OGRECave/ogre/blob/master/BuildingOgre.md) cover other problems.

### Step 3: Build Code_visualiser

Open a terminal in the project folder (the one containing `app`, `src` and `README.md`).

**3.1 Configure**

```
cmake -S app -B build
```

This generates the Visual Studio solution `build\CodeVisualiser.sln`. It should print `Building for: Visual Studio ...` and `Ogre source folder: C:/Ogre/ogre`. If Ogre is not in `C:\Ogre`, or CMake says it cannot find OGRE, give it the path to Ogre's SDK folder:

```
cmake -S app -B build -DOGRE_DIR=C:/Ogre/out/build/sdk/CMake
```

Replace the path with your own folder that contains `OGREConfig.cmake`. If CMake also warns that it cannot find the Ogre source folder, add the folder that contains `Media`:

```
cmake -S app -B build -DOGRE_DIR=C:/Ogre/out/build/sdk/CMake -DOGRE_SOURCE_ROOT=C:/Ogre/ogre
```

**3.2 Build**

```
cmake --build build --config Release
```

The executable is `build\Release\Code_visualiser.exe`. The build copies the Ogre DLLs next to it.

Options for the configure command (add them, for example `cmake -S app -B build -DOGRE_DIR=...`):

| Option | Meaning |
|---|---|
| `OGRE_DIR` | Folder with `OGREConfig.cmake` (default `C:/Ogre/out/build/sdk/CMake` on Windows) |
| `OGRE_SOURCE_ROOT` | Ogre source folder (the one with `Media`). Found automatically |
| `FV_USER_PROGRAM` | Instrumented source file(s) to show instead of the demo |
| `CV_BUILD_TESTS` | Build the tests (default ON) |
| `CV_TEST_CONFIG` | Also build `config_test` (needs Ogre; default OFF) |

### If something goes wrong

| Message | Cause and fix |
| --- | --- |
| `Cannot open include file: 'd3dx9.h'` while building Ogre | Ogre's DirectX 9 renderer needs the old DirectX SDK. Add `-DOGRE_BUILD_RENDERSYSTEM_D3D9=OFF` in step 2.2 and run 2.2 to 2.4 again |
| `Could not find a package configuration file provided by "OGRE"` | Ogre's install step (2.4) did not finish, or Ogre is elsewhere. Check that `OGREConfig.cmake` exists, then pass `-DOGRE_DIR=...` as in step 3.1 |
| Same message after you fixed Ogre | A stale `build` folder remembers the failure. Delete it (`rmdir /s /q build`) and run 3.1 again |
| `NMAKE : fatal error U1052: file 'Makefile' not found` | CMake chose NMake instead of Visual Studio (usually because CMake is too old for your Visual Studio). Install a newer CMake, or delete `build` and add `-G "Visual Studio 18 2026" -A x64` (or `17 2022`) to the configure command; the name must be in the list at the end of `cmake --help` |

## Running the tool

Run `build\Release\Code_visualiser.exe`. Without options it runs the built-in demo, which asks for a number in the **console window** (type it there, not in the Ogre window).

| Option | Meaning |
|---|---|
| `--speed N` | Start speed of the animation (default 1) |
| `--limit N` | How far the program may run ahead of the animation (default 8) |
| `--quiet` | Do not print every event in the console |
| `--save FILE` | Write the recorded run to FILE when the window is closed |
| `--replay FILE` | Play a saved run without running the program |
| `--help`, `-h` | List the options |

Example: `Code_visualiser.exe --speed 4 --quiet`

### Controls

| Key or button | What it does |
|---|---|
| Pause button or SPACE | Pause or resume |
| Mouse wheel | Scroll up and down |
| Ctrl + wheel, or zoom buttons | Zoom |
| Hold left or right mouse button and move | Drag the view |
| F | Follow the latest shape again |
| L | Show or hide the colour legend |
| `speed` buttons, or + and - | Halve or double the speed |
| `step` buttons, or LEFT and RIGHT | Go one event back or forward |
| `restart` button, or R | Play the whole run again |
| C | Show or hide the call stack panel |
| `export png` button, or E | Save the current view (without the buttons and panels) as `screenshots\view_<date>_<time>.png` in the project folder |
| `save run` button, or S | Save the run to `runs\run_<date>_<time>.fvrun` in the project folder |
| ESC | Quit |

### Save and replay a run

Press S to save a run, then play it later without the program:

    Code_visualiser.exe --replay runs\run_20261105_150000.fvrun

A replay works like a finished live run: pause, speed, step, restart and export all work.

### Run your own program

The program is compiled into the visualiser, so a new program needs a new build. This needs Python and Clang (step 1).

1. Instrument it. This writes `prog_instrumented.cpp` next to your file and does not change the original:

       python instrumenter\fv_instrument.py C:\path\to\prog.cpp

   Include paths, defines and the language standard are given as when compiling:
   `python instrumenter\fv_instrument.py prog.cpp -Iinclude -DDEBUG --std c++20`.
   Other options: `-o FILE` (output name), `--clang PATH`, `--header-dir DIR`, `-v`.
   More in `instrumenter/README.md`.

2. Build the visualiser with that file instead of the demo:

       cmake -S app -B build -DFV_USER_PROGRAM=C:/path/to/prog_instrumented.cpp
       cmake --build build --config Release

3. Run `Code_visualiser.exe` as above. To go back to the demo, run CMake again with `-DFV_USER_PROGRAM=` and build.

Examples are in `instrumenter/examples`.

### Settings

Colours are in `app/config.cfg` as `r,g,b` values from 0 to 1. A missing line uses its default. The file is found from any working directory: the tool looks in the current folder, then in `app/`, then in the project folder. The start-up report shows which file was used.
Restart the program after editing. `maxIterationsShown` sets how many loop iterations are drawn (0 = all).

### Tests

On Windows, `run_tests.bat` builds everything, runs all tests and saves the results in `test_results.txt`.
Or run `ctest -C Release --output-on-failure` in the `build` folder.

| Test | What it checks |
|---|---|
| `demo_test` | The demo reports exactly the expected events |
| `markers_test` | The markers, loop counting, streamed values, stop request and `LiveSession` |
| `backlog_test` | Speed, pause, limit and waiting between program and renderer |
| `eventlog_test` | The event log, its read position and the call stack tracker |
| `input_test` | Stopping a program that waits for console input |
| `shapes_test` | The drawing helpers and the stroke font |
| `runfile_test` | Saving and loading a run, including damaged files |
| `speed_test` | How much slower the markers make `fib(27)` |
| `instrumenter_test` | The instrumenter; needs Python and Clang, skipped without them |
| `config_test` | The config file reader; needs `-DCV_TEST_CONFIG=ON`, then run `build\tests\Release\config_test.exe` |

## What the tool does

Code_visualiser shows what a C++ program does while it runs. The program runs on one thread, and a
window (made with Ogre 3D) draws it as an animated flowchart on another: statements, `if` and `switch`
forks, loops, function calls (as boxes), input and output appear as the program reaches them, and a
pulsing marker shows where execution is. The program is held back so it never runs far ahead of the
animation, and you can pause, change the speed, step back and forward, and save a run to replay later.

To show your own program, the instrumenter (`instrumenter/`) adds markers to a copy of your file; no
other change to your code is needed. Only Windows with Visual Studio and Ogre 14 has been tried.

| Folder | What is in it |
|---|---|
| `app/` | `CMakeLists.txt` and the `.cfg` files |
| `include/`, `src/` | Header and source files |
| `tests/` | The tests |
| `instrumenter/` | The tool that adds the markers to your own C++ file |
| `runs/`, `screenshots/` | Where saved runs and screenshots go |

`run_tests.bat` in the project folder builds and tests everything.
