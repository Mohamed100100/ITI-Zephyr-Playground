# Zephyr Kconfig — Complete Guide to Build-Time Configuration

> **Scope:** This README covers **Part 1 of the lesson — Kconfig**, the
> configuration system Zephyr uses to decide *which features are compiled into
> your firmware and which are not*. Part 2 (Device Tree — hardware description)
> is out of scope here.
>
> **The big idea:** instead of hard-encoding features or relying on run-time
> decisions, Zephyr's scalable configuration system gives you full control and
> allows you to scale your solution. The same application can behave completely
> differently just by changing configuration options — **without touching the
> source code**. This document explains *how* it works and *why* it is one of
> the most important concepts related to Zephyr.

---

## Table of Contents

1. [What is Kconfig?](#1-what-is-kconfig)
2. [Why Zephyr needs Kconfig](#2-why-zephyr-needs-kconfig)
3. [Build-time vs run-time configuration](#3-build-time-vs-run-time-configuration)
4. [Anatomy of a Kconfig file — every keyword explained](#4-anatomy-of-a-kconfig-file--every-keyword-explained)
5. [Kconfig priority hierarchy in Zephyr](#5-kconfig-priority-hierarchy-in-zephyr)
6. [Configuration tools: `menuconfig` and `guiconfig`](#6-configuration-tools-menuconfig-and-guiconfig)
7. [The build flow: how Kconfig becomes C macros](#7-the-build-flow-how-kconfig-becomes-c-macros)
8. [Demo application: Kconfig in practice](#8-demo-application-kconfig-in-practice)
9. [Modifying behavior with `menuconfig` — no source changes](#9-modifying-behavior-with-menuconfig--no-source-changes)
10. [Troubleshooting & debugging configuration](#10-troubleshooting--debugging-configuration)
11. [Common pitfalls](#11-common-pitfalls)
12. [Quick-reference cheatsheet](#12-quick-reference-cheatsheet)
13. [Key takeaways](#13-key-takeaways)

---

## 1. What is Kconfig?

**Kconfig is the configuration system used by Zephyr to decide what features are
compiled into your firmware and what is not.**

It originally comes from the **Linux kernel**. Zephyr reused the idea because it
**scales very well for complex systems** — the same mechanism that lets a Linux
kernel with 30,000+ options stay manageable also works for a real-time operating
system with hundreds of boards, drivers, and subsystems.

### 🔑 The single most important point

**Kconfig works only at build time.**

```
┌────────────────────────────────────────────────────────┐
│  CONFIG OPTION = y   →  Zephyr INCLUDES the code       │
│  CONFIG OPTION = n   →  the code is NOT compiled       │
│                           (not linked, not present)    │
└────────────────────────────────────────────────────────┘
```

When you disable an option, its code is not compiled at all — **no performance
and no memory wasted**. The feature literally does not exist in the final binary.

---

## 2. Why Zephyr needs Kconfig

Embedded systems are very different from desktop systems:

| Resource | Desktop | Embedded |
|---|---|---|
| Flash | GBs | Often a few hundred KB |
| RAM | GBs | Often tens/hundreds of KB |
| CPU | Multi-core GHz | Often single-core, tens of MHz |

And more importantly: **not every product needs the same features.**

- Not every product needs **networking**.
- Not every product needs **Bluetooth**.
- Not every product needs a **shell**.

Kconfig solves this problem by making decisions **at build time, not while the
system is running**. This keeps the firmware:

- **Small** — only what you need gets compiled in.
- **Predictable** — the feature set is fixed and known before flashing.
- **Efficient** — no run-time checks, no dead code, no wasted cycles.

---

## 3. Build-time vs run-time configuration

It is worth being explicit about the difference, because it is the core of the lesson:

| Aspect | Build-time (Kconfig) | Run-time (e.g. `if` in code) |
|---|---|---|
| Decision moment | When you compile | While the chip is running |
| Disabled code | **Removed from binary** | Still in flash, just skipped |
| Cost when disabled | **Zero** (no flash, no RAM, no CPU) | Flash/RAM still occupied |
| How it's expressed | `CONFIG_XXX=y/n` in `.config` + `#if defined(CONFIG_XXX)` in C | Ordinary `if (flag)` statements |
| Can change after flashing? | No (requires rebuild) | Yes |

Zephyr uses Kconfig for the "does this feature exist at all" question, and keeps
run-time decisions only for things that genuinely vary at run time.

---

## 4. Anatomy of a Kconfig file — every keyword explained

Below is a Kconfig file from the Zephyr kernel, annotated line by line:

```kconfig
menu "Kernel Options"
# ─────────────────────────────────────────────────────────────────
# `menu` is just a VISUAL GROUP used by menuconfig to organize
# related options. It has no functional effect on the build.
# ─────────────────────────────────────────────────────────────────

config MULTITHREADING
    bool "Multi-threading"
# ─────────────────────────────────────────────────────────────────
# `config` defines a CONFIGURATION SYMBOL (here: CONFIG_MULTITHREADING).
# The `bool` keyword means this option is either ON or OFF and it
# appears as a CHECKBOX in menuconfig.
# ─────────────────────────────────────────────────────────────────

    depends on ARCH_HAS_MULTITHREADING
# ─────────────────────────────────────────────────────────────────
# `depends on`: this option is only VISIBLE/SELECTABLE if
# ARCH_HAS_MULTITHREADING is enabled. If the architecture does not
# support it, the option simply does not appear (or is shown grayed
# out) in menuconfig, and any config file assigning it a value gets
# a warning/error.
# ─────────────────────────────────────────────────────────────────

    default y
# ─────────────────────────────────────────────────────────────────
# `default`: the value used when you don't change anything.
# Here, multithreading is on unless the user explicitly turns it off.
# ─────────────────────────────────────────────────────────────────

    select SCHED_CPU_MASK
# ─────────────────────────────────────────────────────────────────
# `select`: automatically ENABLES other required options when this
# one is turned on. Turning on MULTITHREADING forces SCHED_CPU_MASK
# on too — the user doesn't have to know about that dependency.
# ─────────────────────────────────────────────────────────────────

    help
      Enable multi-threading support. When enabled, the kernel
      provides threads, scheduling, and synchronization primitives.
# ─────────────────────────────────────────────────────────────────
# `help`: explains what the option does and how it should be used.
# Shown at the bottom of the menuconfig window (press `?` on an
# option to see it).
# ─────────────────────────────────────────────────────────────────

endmenu
```

### 4.1 Complete keyword reference

| Keyword | Syntax example | Meaning |
|---|---|---|
| `menu` / `endmenu` | `menu "Title"` ... `endmenu` | Visual grouping in menuconfig. No functional effect. |
| `config` | `config MY_SYMBOL` | Defines a symbol. In C it becomes `CONFIG_MY_SYMBOL`. |
| `bool` | `bool "prompt string"` | Boolean: on/off. Checkbox in menuconfig. |
| `int` | `int "prompt"` | Integer value (e.g. buffer sizes). |
| `hex` | `hex "prompt"` | Hexadecimal value. |
| `string` | `string "prompt"` | Free text (e.g. a device name). |
| `default` | `default y` | Value when the user doesn't set one. |
| `depends on` | `depends on FOO` | Option only visible/enabled if `FOO=y`. |
| `select` | `select BAR` | Forces `BAR=y` whenever this option is `y`. |
| `imply` | `imply BAZ` | Sets `BAZ=y` by default, but the user can still override to `n`. (Softer than `select`.) |
| `choice` / `endchoice` | pick-one-of-many group | Radio buttons in menuconfig — exactly one member is `y`. |
| `prompt` | `prompt "text"` (or inline after the type) | The human-readable label. |
| `range` | `range 1 100` | Valid range for `int`/`hex` options. |
| `help` | `help` + indented text | Documentation shown in menuconfig. |
| `comment` | `comment "text"` | Display-only line in menuconfig. |
| `source` | `source "Kconfig.zephyr"` | Includes another Kconfig file. |

> **Naming convention:** a Kconfig symbol `MULTITHREADING` becomes
> `CONFIG_MULTITHREADING` in C and in `.config` files. The `CONFIG_` prefix is
> added automatically — you never write it in the Kconfig file itself.

### 4.2 Application Kconfig from the demo

```kconfig
source "Kconfig.zephyr"      # Gives access to ALL core Zephyr configuration
                             # symbols (kernel, drivers, subsystems, ...)

menu "Application Configuration"      # Group app-specific options; they will
                                      # appear under this menu in menuconfig.

config APP_ENABLE_BACKGROUND_WORK
    bool "Enable background work"
    default y
    help
      Enables a background task using Zephyr's workqueue API. The task is
      active as soon as the application is built.

config APP_ENABLE_DEBUG_LOGS
    bool "Enable debug logs for background task"
    depends on APP_ENABLE_BACKGROUND_WORK      # ← only available when the
    default n                                  #   background task is enabled

config APP_ENABLE_SHELL
    bool "Enable Zephyr shell"
    default n
    help
      Disabled by default to keep the system minimal unless shell access
      is needed.

endmenu
```

**What this buys you:**
- `APP_ENABLE_BACKGROUND_WORK` is **on by default** — the background task is
  active as soon as you build.
- `APP_ENABLE_DEBUG_LOGS` **depends on** the background work option — it can
  only be enabled if the task exists. menuconfig will gray it out otherwise.
- `APP_ENABLE_SHELL` is **off by default** to keep the system minimal.

---

## 5. Kconfig priority hierarchy in Zephyr

Configuration can come from several places. When the same symbol is assigned
different values, **the higher-priority source wins**:

```
        ┌──────────────────────────────────────────────┐
        │  1. Overlay configuration files   ← WINS     │  e.g. -DOVERLAY_CONFIG=extra.conf,
        │     (highest priority — override             │      *.overlay fragments,
        │      everything below)                       │      board-specific .conf files
        ├──────────────────────────────────────────────┤
        │  2. prj.conf                                 │  ← where YOUR APPLICATION
        │     (application configuration)              │    customizes behavior
        ├──────────────────────────────────────────────┤
        │  3. Board defconfig                          │  ← every board enables what
        │     (board defaults, e.g. boards/.../*.conf) │    it needs to boot & run
        ├──────────────────────────────────────────────┤
        │  4. Zephyr Kconfig files        ← loses      │  ← kernel features, drivers,
        │     (kernel & subsystem defaults)            │    subsystems (their `default`s)
        └──────────────────────────────────────────────┘
```

- **Zephyr Kconfig files** define kernel features, drivers, and subsystems —
  each with its own `default` value.
- **Board defconfig** — every board enables what it needs to boot and run
  correctly (its UART, its flash driver, its clock settings...).
- **prj.conf** — where the application customizes behavior. This is the file
  you edit most often.
- **Overlay configuration files** have the **highest priority** and override
  everything below. Used for board variants, CI builds, and temporary tweaks.

---

## 6. Configuration tools: `menuconfig` and `guiconfig`

Zephyr provides two interactive tools to configure Kconfig options. **Both do
exactly the same thing: they modify the final `.config` file used at build time.**

### 6.1 `menuconfig` — the most commonly used one

```bash
west build -t menuconfig
```

- Runs directly in the terminal.
- Provides a **text-based interface**.
- You can navigate through **all** config menus (kernel, drivers, subsystems,
  and your own application options).
- Enable or disable config options **immediately**.
- It understands **dependencies and prevents invalid configurations** — an
  option whose dependency is unmet is shown grayed out and cannot be enabled.

#### Essential menuconfig keys

| Key | Action |
|---|---|
| `↑` / `↓` | Navigate the menu |
| `Enter` | Enter a submenu |
| `Esc` `Esc` | Exit a submenu / quit (asks to save) |
| `Y` / `N` / `M` | Set the highlighted option to yes / no / module |
| `Space` | Toggle a bool option |
| `?` | Show help for the highlighted option |
| `/` | Search for a symbol (very useful in a tree with thousands of options) |
| `S` | Save |
| `Q` | Quit |

### 6.2 `guiconfig`

- The **same system with a graphical interface** (runs in a window instead of
  the terminal).
- Useful if you prefer clicking through menus, or need to see long help texts
  comfortably.

> ⚠️ Whichever tool you use, the output is the same: a `.config` file that the
> build system consumes. Neither tool changes your source code.

---

## 7. The build flow: how Kconfig becomes C macros

Understanding this pipeline explains *everything* about Kconfig:

```
  Kconfig files (kernel, board, app)
        │
        ▼
  ┌─────────────┐   merged by priority:      ┌──────────────┐
  │ prj.conf    │── board defconfig ────────▶│  .config     │
  │ overlays    │                            │  (resolved,  │
  └─────────────┘                            │   dependency-│
        │                                    │   checked)   │
        ▼                                    └──────┬───────┘
  Kconfig parser (kconfiglib)                       │
        │                                           ▼
        │                              ┌─────────────────────────┐
        │                              │ autoconf.h              │
        └─────────────────────────────▶│ (generated header)      │
                                       │ #define CONFIG_APP_...  │
                                       └───────────┬─────────────┘
                                                   │ #include'd (directly or
                                                   ▼  via Zephyr headers)
                                       ┌─────────────────────────┐
                                       │ Compilation             │
                                       │ #if defined(CONFIG_X)   │
                                       │   → code included       │
                                       │ #else                   │
                                       │   → code REMOVED        │
                                       └─────────────────────────┘
```

1. All Kconfig **definitions** (from Zephyr, the board, and your app) are parsed.
2. All **value assignments** are merged according to the priority hierarchy
   (Section 5). Dependencies are checked; invalid assignments produce warnings.
3. The result is written to **`build/.config`**.
4. A header generator turns `.config` into **`build/zephyr/include/generated/autoconf.h`** —
   every `CONFIG_XXX=y` becomes `#define CONFIG_XXX 1`.
5. Your C code tests these macros with `#if defined(CONFIG_XXX)`. The
   preprocessor includes or removes code **before compilation** — so disabled
   features cost literally nothing.

---

## 8. Demo application: Kconfig in practice

**Goal:** not the application itself, but to clearly see how changing Kconfig
options directly changes firmware behavior **at build time**.

### 8.1 Project layout

```
kconfig_demo/
├── CMakeLists.txt      ← build system entry point
├── Kconfig             ← application configuration options
├── prj.conf            ← application configuration values
└── src/
    └── main.c          ← application code with #if CONFIG guards
```

### 8.2 Step-by-step plan

1. Create the application with a minimal `CMakeLists.txt`.
2. Create a `Kconfig` file with application-specific options.
3. Create `src/main.c` using `#if defined(CONFIG_...)` guards.
4. Fill `prj.conf` with default option values.
5. Build, flash, and observe which features are enabled.
6. Inspect the active configuration (`.config` / `autoconf.h`) to understand
   what is actually being compiled into the firmware.
7. Modify options via `menuconfig`, rebuild, and observe the behavior change —
   **without touching the source code**.

### 8.3 `CMakeLists.txt` — line by line

```cmake
cmake_minimum_required(VERSION 3.20.0)
# ↑ The minimal CMake version required to build Zephyr applications.

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
# ↑ Locate Zephyr using the ZEPHYR_BASE environment variable,
#   making sure Zephyr is available before continuing.

project(kconfig_demo)
# ↑ Declares the project name, which defines this application
#   inside the Zephyr build system.

target_sources(app PRIVATE src/main.c)
# ↑ Tells the build which source files belong to the application —
#   here, main.c inside the src directory.
```

### 8.4 Application `Kconfig`

(Shown and explained in Section 4.2 — three options: background work,
debug logs with a dependency, and shell, all under an "Application
Configuration" menu sourced from `Kconfig.zephyr`.)

### 8.5 `src/main.c` — line by line

```c
#include <zephyr/kernel.h>
/* ↑ Zephyr Kernel header: gives access to the core kernel APIs
     such as timers and workqueues. */

#include <zephyr/logging/log.h>
/* ↑ Provides the logging infrastructure used throughout the application. */

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);
/* ↑ Registers a log module named "app" and sets the default log level
     to INFO (verbosity controlled via CONFIG_LOG_* options). */

#if defined(CONFIG_APP_ENABLE_BACKGROUND_WORK)
static struct k_work_delayable background_work;
/* ↑ Declared only if background work is enabled. This object represents
     a task that can be SCHEDULED TO RUN AFTER A DELAY. */
#endif

#if defined(CONFIG_APP_ENABLE_SHELL)
#include <zephyr/shell/shell.h>
/* ↑ Shell header included ONLY when the shell feature is active —
     makes shell-related APIs available at compile time. */
#endif

#if defined(CONFIG_APP_ENABLE_BACKGROUND_WORK)
static void background_task(struct k_work *work)
{
    /* Callback executed by the Zephyr workqueue. */

#if defined(CONFIG_APP_ENABLE_DEBUG_LOGS)
    LOG_INF("Running background task - debug mode");
    /* ↑ Detailed log message when debug logs are enabled. */
#else
    LOG_INF("Running background task");
    /* ↑ Simpler message otherwise — shows how config controls
         behavior at COMPILE time. */
#endif

    k_work_reschedule(&background_work, K_SECONDS(5));
    /* ↑ Reschedule the same work item to run again after 5 seconds,
         creating a PERIODIC background activity. */
}
#endif

int main(void)
{
    LOG_INF("Application started");

#if defined(CONFIG_APP_ENABLE_BACKGROUND_WORK)
    k_work_init_delayable(&background_work, background_task);
    /* ↑ Initialize the delayable work structure. */
    k_work_schedule(&background_work, K_SECONDS(1));
    /* ↑ Schedule it to run for the FIRST time after one second. */
#endif

#if defined(CONFIG_APP_ENABLE_SHELL)
    LOG_INF("Zephyr shell enabled for diagnostics and interaction");
#endif

    return 0;
}
```

### 8.6 `prj.conf` — first run

```conf
CONFIG_APP_ENABLE_BACKGROUND_WORK=y   # Background work task is enabled and
                                      # will be compiled into the application.
CONFIG_APP_ENABLE_DEBUG_LOGS=y        # Because of this, the background task
                                      # prints additional log messages.
CONFIG_APP_ENABLE_SHELL=n             # Shell is NOT included in the build.
CONFIG_LOG=y                          # Logging subsystem enabled.
CONFIG_LOG_DEFAULT_LEVEL=3            # INFO level verbosity (0=NONE, 1=ERR,
                                      # 2=WRN, 3=INF, 4=DBG).
```

### 8.7 Build, flash, and observe

```bash
west build -b <your_board> .        # configure & build
west flash                          # flash to the board
```

Console output — the background task runs in **debug mode**, exactly as configured:

```
*** Booting Zephyr OS build ... ***
Application started
Running background task - debug mode
Running background task - debug mode
... (every 5 seconds)
```

### 8.8 Proof that it works at build time: `.config` and `autoconf.h`

After building, inspect the generated files in the build folder:

**`build/.config`** (excerpt):

```conf
CONFIG_APP_ENABLE_BACKGROUND_WORK=y
CONFIG_APP_ENABLE_DEBUG_LOGS=y
CONFIG_APP_ENABLE_SHELL=n
```

**`build/zephyr/include/generated/autoconf.h`** (excerpt):

```c
#define CONFIG_APP_ENABLE_BACKGROUND_WORK 1
#define CONFIG_APP_ENABLE_DEBUG_LOGS 1
/* CONFIG_APP_ENABLE_SHELL is simply ABSENT — which is why
   #if defined(CONFIG_APP_ENABLE_SHELL) is false in main.c. */
```

These files let you verify exactly what is compiled into your firmware.

---

## 9. Modifying behavior with `menuconfig` — no source changes

Now the key experiment: change the behavior **only** through configuration.

```bash
west build -t menuconfig
```

1. Navigate the menu (there are many options related to the kernel, drivers,
   and others — but concentrate on the application itself):
   **Application Configuration**.
2. **Enable** `Enable Zephyr shell` (`APP_ENABLE_SHELL=y`).
3. **Disable** `Enable background work` (`APP_ENABLE_BACKGROUND_WORK=n`).
   - Notice that `Enable debug logs for background task` **disappears/grayed out**
     automatically — menuconfig enforces the `depends on` and prevents an
     invalid configuration.
4. Save to `.config` (`S`) and quit (`Q`).
5. Rebuild and flash. For example, with the J-Link runner on a Raspberry Pi Pico:

   ```bash
   west flash --runner jlink
   ```

**Console output after the change:**

```
*** Booting Zephyr OS build ... ***
Application started
Zephyr shell enabled for diagnostics
uart:~$
```

The background task and its debug logs are **completely gone from the binary**,
and the interactive shell is now present. **Same source code, completely
different firmware** — purely because Kconfig options changed at build time.

---

## 10. Troubleshooting & debugging configuration

| Symptom | Where to look / what to do |
|---|---|
| "My option isn't visible in menuconfig" | It likely has an unmet `depends on`. Enable the dependency first, or search with `/` to see its dependencies. |
| "I set an option in prj.conf but it's still off" | A higher-priority source overrides it (Section 5), or its dependency isn't met. Check the build log — Kconfiglib prints warnings for unmet deps. |
| "My #if defined(CONFIG_X) is false even though I enabled X" | Make sure `CONFIG_X=y` actually landed in `build/.config`, then check `autoconf.h`. Do a clean rebuild (`west build -p` / pristine build). |
| "Everything seems stale" | Build folder holds the old `.config`. Force a pristine build: `west build -p always` (or delete `build/`). |
| "I want to find which option controls feature Y" | `menuconfig` → press `/`, type a keyword from the prompt or symbol name. |
| "The symbol name vs config name confusion" | In Kconfig you write `config FOO`; everywhere else (`.conf` files, C code) it is `CONFIG_FOO`. |
| "Board defconfig vs prj.conf conflict" | `prj.conf` wins for plain builds. For temporary overrides use an extra overlay: `west build -- -DOVERLAY_CONFIG=debug.conf`. |

---

## 11. Common pitfalls

1. **Expecting run-time behavior from a build-time option.** Kconfig decides
   *existence*, not *runtime state*. Once flashed, you can't turn a feature on
   without rebuilding.
2. **Forgetting the `CONFIG_` prefix** in `.conf` files and C code
   (`CONFIG_APP_ENABLE_SHELL`, never `APP_ENABLE_SHELL`).
3. **Ignoring `depends on`.** Assigning a value to a symbol whose dependencies
   are unmet produces warnings and the assignment is dropped.
4. **Overusing `select`.** `select` forces options on even if the user turned
   them off elsewhere — prefer `depends on` or `imply` when possible.
5. **Editing `build/.config` by hand.** It is generated; your changes will be
   lost. Edit `prj.conf` or an overlay instead, and use `menuconfig` as the
   interactive editor.
6. **Not checking `.config` / `autoconf.h`.** When in doubt, read the resolved
   configuration — it is the ground truth of what the build is doing.

---

## 12. Quick-reference cheatsheet

```bash
# Build (pristine when config changed a lot)
west build -b <board> .
west build -p always            # pristine build

# Interactive configuration (both edit build/.config)
west build -t menuconfig        # terminal UI
west build -t guiconfig         # graphical UI

# Flash
west flash
west flash --runner jlink       # e.g. Raspberry Pi Pico via J-Link

# Inspect the resolved configuration
cat build/.config
cat build/zephyr/include/generated/autoconf.h

# Extra overlay (highest priority)
west build -- -DOVERLAY_CONFIG=extra.conf
```

```c
/* C code pattern */
#if defined(CONFIG_MY_FEATURE)
    /* compiled in only when CONFIG_MY_FEATURE=y */
#endif
```

```kconfig
# Kconfig pattern
config MY_FEATURE
    bool "Enable my feature"
    depends on DEPENDENCY
    default n
    select AUTO_ENABLED_OPTION
    help
      Describe what it does and when to use it.
```

---

## 13. Key takeaways

- **Kconfig = build-time feature selection.** It comes from the Linux kernel and
  scales very well for complex systems.
- **Kconfig works only at build time.** Disabled code is not compiled — no
  flash, no RAM, no CPU wasted. Small, predictable, efficient firmware.
- Embedded systems need it because of limited resources and because **not every
  product needs the same features** (network, Bluetooth, shell, ...).
- Core keywords: `menu`, `config`, `bool`, `depends on`, `default`, `select`,
  `help`.
- Options are resolved through a **priority hierarchy**: Zephyr Kconfig → board
  defconfig → `prj.conf` → overlay files (highest).
- `menuconfig` and `guiconfig` are just editors for `.config`; they also enforce
  dependencies and prevent invalid configurations.
- In C, options become `CONFIG_*` macros via the generated `autoconf.h`, and
  `#if defined(CONFIG_...)` includes or removes code before compilation.
- **The same application can behave completely differently by changing
  configuration options without touching the source code** — enable the shell,
  disable the background task — and the proof is in `build/.config` and
  `autoconf.h`. That is the power of Kconfig.

---

*Next part of the lesson: **Device Tree** — describing the hardware.*
