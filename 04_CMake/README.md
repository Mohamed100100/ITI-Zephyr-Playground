# 04 — CMake: How Zephyr Builds Your Project

This guide explains **CMake** in Zephyr — the tool that reads your project and
starts the build. It is short and practical.

After this lesson you will understand:

- what CMake does in a Zephyr build,
- the **5 important lines** in every `CMakeLists.txt`,
- the project folders `src/`, `inc/`, `build/`,
- how to build **from your project folder** when the Zephyr workspace is
  somewhere else.

> **Learning path:** `03 Create Project` → **`04 CMake`** →
> `05 Kconfig vs Devicetree` → `06 Kconfig deep dive` → `07 Custom Module` /
> `08 Devicetree` / `09 Create Custom Devicetree`.
>
> **Main reference:** [Zephyr documentation](https://docs.zephyrproject.org/)

---

## 1. What CMake does

You type one command:

```bash
west build -b <board> .
```

Behind the scenes, the tools run in this order:

```
west  →  CMake  →  Kconfig + Devicetree  →  Ninja  →  firmware
```

- **west** — Zephyr's helper tool. It finds the board and starts the build.
- **CMake** — reads `CMakeLists.txt`, finds the source files, and creates the
  build files.
- **Kconfig** — decides which software features are compiled.
- **Devicetree** — describes the hardware (pins, buses, devices).
- **Ninja** — compiles and links everything into the firmware.

So CMake is the **entry point of the build**. It does not compile code itself;
it prepares the build so Ninja can compile it.

> **Simple idea:** `CMakeLists.txt` = "here is my project: its name, its source
> files, and its include folders."

---

## 2. The project structure

A clean Zephyr project looks like this:

```
my_project/
├── CMakeLists.txt     ← the build entry point (CMake reads this)
├── prj.conf           ← software features (Kconfig)
├── src/
│   └── main.c         ← your C code
├── inc/
│   └── app_config.h   ← your own headers (optional)
└── build/             ← build output (created by west build, do not commit)
```

| Folder / file | Job |
|---|---|
| `CMakeLists.txt` | Tells CMake what to build |
| `prj.conf` | Turns software features on/off (Kconfig) |
| `src/` | Your `.c` source files |
| `inc/` | Your `.h` header files (optional) |
| `build/` | Generated output: `.elf`, `.bin`, `.config`, `zephyr.dts` |

The `inc/` folder is optional. `cmake/` is also optional and only useful for
very large projects — see Section 4. Small examples can put everything in
`src/`.

---

## 3. The 5 important lines in `CMakeLists.txt`

Here is a complete, working `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_project)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
```

That is the whole file. Only five lines matter:

| # | Line | What it does |
|---|---|---|
| 1 | `cmake_minimum_required(VERSION 3.28.0)` | Says which CMake version is needed. Zephyr 4.4 needs **3.28 or newer**. If your CMake is older, the build stops with a clear error. |
| 2 | `find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})` | **Loads Zephyr.** It uses the `ZEPHYR_BASE` environment variable to find the Zephyr source. `REQUIRED` means "stop if Zephyr is not found". |
| 3 | `project(my_project)` | Gives your application a name. |
| 4 | `target_include_directories(app PRIVATE ...)` | Adds the `inc/` folder to the compiler include path (optional). |
| 5 | `target_sources(app PRIVATE src/main.c)` | Adds your source files to the `app` target. Add more files like this: `target_sources(app PRIVATE src/led.c src/button.c)`. |

> **Line 2 is the most important.** It connects your project to Zephyr.
> Without it, nothing else works.

### Line 1 — `cmake_minimum_required`

```cmake
cmake_minimum_required(VERSION 3.28.0)
```

- Always the **first** line.
- It protects you: if the CMake on your machine is too old, you get a clear
  message instead of strange errors later.
- Check your version with `cmake --version`.

### Line 2 — `find_package(Zephyr ...)`

```cmake
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
```

- Finds and loads the Zephyr build system.
- `ZEPHYR_BASE` points to the `zephyr/` folder inside your workspace.
- `HINTS` tells CMake where to look first.
- This line also creates the `app` target that line 5 uses.

If you see `find_package(Zephyr) not found`, run `west zephyr-export` once (see
[lesson 02](../02_Installation_Guide_What_Gets_Installed/README.md)).

### Line 3 — `project`

```cmake
project(my_project)
```

- Names the application.
- Must come **after** `find_package(Zephyr)`, because Zephyr sets up the build
  first.

### Line 4 — `target_include_directories` (optional)

```cmake
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
```

- Adds your `inc/` folder to the compiler include path.
- After this, `src/main.c` can write `#include <app_config.h>`.
- It is optional — remove the line if you have no headers.
- For very large projects you can move this into a `cmake/` helper file instead
  (Section 4).

### Line 5 — `target_sources`

```cmake
target_sources(app PRIVATE src/main.c)
```

- Adds source files to the `app` target.
- `PRIVATE` means "these files belong only to this application".
- Add as many files as you want:

```cmake
target_sources(app PRIVATE
    src/main.c
    src/led.c
    src/button.c
)
```

---

## 4. Optional: the `cmake/` folder (advanced)

The `cmake/` folder holds small helper files. Keep `CMakeLists.txt` short by
moving details here.

Example `cmake/app.cmake`:

```cmake
# Add the project's local include folder so #include <app_config.h> works.
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../inc)
```

- `${CMAKE_CURRENT_LIST_DIR}` is the folder of **this** file
  (`my_project/cmake`).
- `../inc` goes up one level, then into `inc/` → `my_project/inc`.
- `target_include_directories(app PRIVATE ...)` adds that folder to the
  compiler's include search path.

Now `main.c` can simply write:

```c
#include <app_config.h>
```

You could put this line directly in `CMakeLists.txt`. The `cmake/` folder is
just a way to keep things organized.

---

## 5. The `inc/` folder

`inc/` holds your own header files.

Example `inc/app_config.h`:

```c
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define APP_BLINK_DELAY_MS 500

#endif
```

Example use in `src/main.c`:

```c
#include <app_config.h>

k_msleep(APP_BLINK_DELAY_MS);
```

This keeps numbers and settings in one place. Change the header, rebuild, and
every file that includes it gets the new value.

---

## 6. The `build/` folder

`build/` is created by the first build. It holds generated files:

```
build/
└── zephyr/
    ├── zephyr.elf          ← the firmware
    ├── zephyr.bin
    ├── zephyr.hex
    ├── zephyr.dts          ← the final merged Devicetree
    └── .config             ← the final Kconfig
```

Rules for `build/`:

- **Do not edit** files inside it — they are regenerated.
- **Do not commit** it to git. Add `build/` to `.gitignore`.
- Delete it (or use `west build -p always`) when you want a clean build.

If you run `west build` from inside your project folder, the build goes into
`my_project/build/` — right next to your code.

---

## 7. Build from the project folder when the workspace is elsewhere

Your Zephyr workspace (`~/zephyrproject`) and your project can live in
**different places**. You only need to tell CMake where Zephyr is.

### Option A — set `ZEPHYR_BASE` once per terminal

```bash
source ~/zephyrproject/.venv/bin/activate
export ZEPHYR_BASE=~/zephyrproject/zephyr

cd ~/projects/my_project
west build -b blackpill_f401cc -d build .
```

- `-d build` puts the output in `my_project/build/`.
- `.` means "the project is the current folder".
- `west build` works from outside the workspace because `ZEPHYR_BASE` is set.

### Option B — pass the board and build dir explicitly

```bash
source ~/zephyrproject/.venv/bin/activate
export ZEPHYR_BASE=~/zephyrproject/zephyr

west build -p always -b blackpill_f401cc -d build /path/to/my_project
```

Use `-p always` to force a clean build.

> **Important:** `west build` must find Zephyr. Either `ZEPHYR_BASE` is set, or
> you run the command from inside the workspace. Setting `ZEPHYR_BASE` lets you
> keep the build inside your project folder.

---

## 8. Common CMake mistakes

| Problem | Cause / fix |
|---|---|
| `CMake 3.28.0 or higher is required` | Your CMake is old. Install a newer one (`pip install cmake` inside the venv) or upgrade the system CMake. |
| `find_package(Zephyr) not found` | `ZEPHYR_BASE` is not set, or you did not run `west zephyr-export`. |
| `Cannot find source file: src/main.c` | The path in `target_sources` is wrong. Check the folder and file name. |
| `app_config.h: No such file or directory` | The `inc/` folder is not on the include path. Add `target_include_directories`. |
| `project() ... must be called before ...` | Put `find_package(Zephyr)` before `project()`. |
| Changes to `CMakeLists.txt` are ignored | Do a pristine build: `west build -p always`. |
| `No such file: cmake/app.cmake` | Only applies if you use the optional `cmake/` helper (Section 4). |
| Build output appears in the wrong place | Pass `-d build` (or another folder) to `west build`. |

---

## 9. Important commands

These are the commands you use most often. Run them from your **project
folder** (with `ZEPHYR_BASE` set) or from the Zephyr workspace.

```bash
# --- environment (every new terminal) ---
source ~/zephyrproject/.venv/bin/activate
export ZEPHYR_BASE=~/zephyrproject/zephyr

# --- build ---
west build -b <board> -d build .          # build into ./build
west build -p always -b <board> -d build . # clean build
west build -t menuconfig -d build          # edit Kconfig
west build -t guiconfig -d build           # graphical Kconfig editor
west build -t run -d build                 # run on native_sim / QEMU

# --- flash ---
west flash -d build                        # flash the board
west flash -d build --runner <runner>      # choose a specific runner

# --- inspect the result ---
cat build/zephyr/.config                   # final Kconfig
cat build/zephyr/zephyr.dts                # final merged Devicetree
ls build/zephyr/zephyr.elf                 # the firmware

# --- discovery ---
west boards | grep -i <name>               # find a board target
west --version                             # west version
cmake --version                            # CMake version
ninja --version                            # Ninja version
```

> **Order tip:** activate the venv first, then set `ZEPHYR_BASE`, then build.
> If you open a new terminal, do the first two steps again.

---

## Related documents

- [Create a New Zephyr Project](../03_Create_New_Project/README.md) — the
  `new_project` template with `src/`, `inc/`, `build/`.
- [Kconfig vs Devicetree](<../05_Kconfig_vs_Devicetree_DTS/README.md>) — what
  CMake starts, and how the two configuration systems meet.
- [Kconfig deep dive](../06_KConfig/README.md) — the configuration system.
- [Devicetree deep dive](../08_Devicetree/README.md) — the hardware
  description.
- [Custom Module](<../07_Create_Custom_Module_Digital_To_Volt/README.md>) —
  add your own module to the build.
- [Zephyr documentation](https://docs.zephyrproject.org/) — the main reference.

*Zephyr v4.4.0-rc1 — Ubuntu host.*
