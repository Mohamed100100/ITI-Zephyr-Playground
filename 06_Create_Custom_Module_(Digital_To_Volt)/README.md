# 06 — Create a Custom Zephyr Module: Digital_To_Volt

> **Goal:** build your own out-of-tree Zephyr module from scratch — a
> `Digital_To_Volt` library that converts raw ADC counts to millivolts/volts —
> and use it from a demo app on `native_sim` (no hardware) and on the
> WeAct Black Pill `blackpill_f401cc` (real ADC on PA1).
>
> **Prerequisites:** folders `01`–`05` in this playground (Zephyr intro, install,
> new project, Kconfig vs DTS, Kconfig deep dive). Every command assumes:
> ```bash
> source ~/zephyrproject/.venv/bin/activate
> ```
>
> **Hint — where can the module live?** Anywhere. In this lesson it sits next to
> the app (`digital_to_volt_Module/`), but you can copy that exact folder into
> your Zephyr workspace — e.g. `~/zephyrproject/digital_to_volt_Module` (next to
> the `zephyr/` directory) — and just point the build at it:
> `west build -- -DZEPHYR_EXTRA_MODULES=~/zephyrproject/digital_to_volt_Module .`
> Zephyr only needs the path + the `zephyr/module.yml` inside it.

---

## Table of Contents

1. [What you are building](#1-what-you-are-building)
2. [Workspace layout (all files)](#2-workspace-layout-all-files)
3. [The module, file by file](#3-the-module-file-by-file)
4. [The demo app, file by file](#4-the-demo-app-file-by-file)
5. [Build and run — native_sim (simulate, no hardware)](#5-build-and-run--native_sim-simulate-no-hardware)
6. [Build and flash — Black Pill (real ADC)](#6-build-and-flash--black-pill-real-adc)
7. [How it works — Kconfig + CMake + Devicetree in one build](#7-how-it-works--kconfig--cmake--devicetree-in-one-build)
8. [Customizing](#8-customizing)
9. [Troubleshooting](#9-troubleshooting)
10. [Cheat sheet](#10-cheat-sheet)

---

## 1. What you are building

A proper Zephyr **module** (not just code pasted into `src/`):

* Lives **outside** the `zephyr/` tree, versioned separately.
* Brings its own `Kconfig` (feature flags) + `CMakeLists.txt` (what to compile)
  + public headers + sources.
* Is discovered at configure time via `ZEPHYR_EXTRA_MODULES`.
* The demo app `#include <digital_to_volt/digital_to_volt.h>` and calls
  `digital_to_volt()` — the same way it would use any upstream Zephyr library.

Conversion math (one float helper):

```
volts = (float)digital_value * VREF_MV / RESOLUTION_BITS
```

Examples at VREF=3300 / RES=12: `0 -> 0.000 V`, `6 -> 1650.000 V`, `12 -> 3300.000 V`.

---

## 2. Workspace layout (all files)

```
06_Create_Custom_Module_(Digital_To_Volt)/
├── README.md                        ← this file (full guide)
├── digital_to_volt_Module/                      ← THE MODULE (reusable, board-independent)
│   ├── zephyr/module.yml            ← tells Zephyr "this folder is a module"
│   ├── CMakeLists.txt               ← which .c files to compile, where headers are
│   ├── Kconfig                      ← CONFIG_DIGITAL_TO_VOLT + VREF_MV + RESOLUTION_BITS
│   ├── include/digital_to_volt/
│   │   └── digital_to_volt.h        ← public API (digital_to_volt)
│   └── src/
│       └── digital_to_volt.c        ← implementation (64-bit-safe integer math)
└── app/                             ← DEMO APP (uses the module)
    ├── CMakeLists.txt               ← find_package(Zephyr) + src/main.c
    ├── prj.conf                     ← Kconfig values (GPIO, ADC, LOG, DIGITAL_TO_VOLT_*)
    ├── boards/
    │   └── blackpill_f401cc.overlay ← ADC1 channel 1 (PA1) -> io-channels
    └── src/
        └── main.c                   ← simulate sweep OR real ADC read + LED by threshold
```

---

## 3. The module, file by file

### 3.1 `digital_to_volt_Module/zephyr/module.yml`

```yaml
name: digital_to_volt
build:
  cmake: .
  kconfig: Kconfig
```

* `name` — module identifier shown in CMake logs.
* `build.cmake: .` — "the module's CMake file is the `CMakeLists.txt` next to me".
* `build.kconfig: Kconfig` — "parse my `Kconfig` so the module's symbols exist in menuconfig".

Without this file, `ZEPHYR_EXTRA_MODULES` silently ignores the folder.

### 3.2 `digital_to_volt_Module/CMakeLists.txt`

```cmake
if(CONFIG_DIGITAL_TO_VOLT)

    zephyr_library()


    zephyr_library_sources(
        src/digital_to_volt.c
    )


    zephyr_include_directories(
        include
    )


endif()
```

| Line | Meaning |
|---|---|
| `if(CONFIG_DIGITAL_TO_VOLT) ... endif()` | Plain CMake condition on the Kconfig symbol (`=y` → true). The whole module is compiled only when enabled (`=n` → zero bytes in binary). |
| `zephyr_library()` | Create the module's static library target (no name needed — one library per CMake file). |
| `zephyr_library_sources(src/digital_to_volt.c)` | The single source file of this module. |
| `zephyr_include_directories(include)` | Put `include/` on the compiler path so apps can `#include <digital_to_volt/...>`. |

### 3.3 `digital_to_volt_Module/Kconfig`

```kconfig
menuconfig DIGITAL_TO_VOLT    bool "Digital to Volt"
    default y
    help
      Enable the Digital to Volt module.


if DIGITAL_TO_VOLT


config DIGITAL_TO_VOLT_VREF_MV
    int "Reference voltage in mV"
    default 3300
    help
      Reference voltage used for digital-to-voltage conversion.


config DIGITAL_TO_VOLT_RESOLUTION_BITS
    int "ADC resolution"
    range 6 16
    default 12
    help
      Maximum digital value corresponding to the reference voltage.


endif
```

This is **Kconfig** (see folder `05_KConfig` for the full system):

* `menuconfig X` defines a toggle symbol *and* opens a submenu — in C it is
  `CONFIG_X`, in `prj.conf` it is `CONFIG_X=...`. Here the toggle is
  `CONFIG_DIGITAL_TO_VOLT`.
* `if DIGITAL_TO_VOLT ... endif` — the two options below are only visible /
  settable while the module is enabled. menuconfig enforces it.
* `int` + `default` + `range` give sane Black Pill values (3300 mV, 12-bit).
  `VREF_MV` has no `range` (any positive value allowed); resolution is clamped
  to 6–16 bits.

### 3.4 `digital_to_volt_Module/include/digital_to_volt/digital_to_volt.h` + `src/digital_to_volt.c`

One function — the whole module:

```c
/* digital_to_volt.h */
#ifndef DIGITAL_TO_VOLT_H
#define DIGITAL_TO_VOLT_H

#include <stdint.h>

float digital_to_volt(uint32_t digital_value);

#endif
```

```c
/* digital_to_volt.c */
#include <digital_to_volt/digital_to_volt.h>
#include <zephyr/sys/util.h>

float digital_to_volt(uint32_t digital_value)
{
    return ((float)digital_value *
            CONFIG_DIGITAL_TO_VOLT_VREF_MV) /
           CONFIG_DIGITAL_TO_VOLT_RESOLUTION_BITS;
}
```

How it works:

* The function takes **only the raw ADC code**. Vref and resolution come
  straight from Kconfig (`CONFIG_DIGITAL_TO_VOLT_VREF_MV`,
  `CONFIG_DIGITAL_TO_VOLT_RESOLUTION_BITS`) — they are `#define`s baked in at
  compile time via `autoconf.h`, so no extra arguments are needed.
* Change Vref in `prj.conf` or `menuconfig`, rebuild, and every call uses the
  new value — no C code touched.

---

## 4. The demo app, file by file

### 4.1 `app/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(d2v_demo)
target_sources(app PRIVATE src/main.c)
```

Standard app entry point. The module is **not** copied here — it is found via
`ZEPHYR_EXTRA_MODULES` on the `west build` command line (Section 5/6).

### 4.2 `app/prj.conf` — Kconfig values for this build

```conf
CONFIG_GPIO=y
CONFIG_ADC=y
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
CONFIG_DIGITAL_TO_VOLT=y
CONFIG_DIGITAL_TO_VOLT_VREF_MV=3300
CONFIG_DIGITAL_TO_VOLT_RESOLUTION_BITS=12
```

* First block: Zephyr subsystems (GPIO for LED, ADC for real reads, LOG for output).
* Second block: module options — Vref must match your board's supply (3300 for the
  Black Pill), resolution must match the ADC channel's `zephyr,resolution` in the overlay.
* Verify after a build: `cat build/.config | grep DIGITAL_TO_VOLT`, `cat build/zephyr/include/generated/autoconf.h | grep DIGITAL_TO_VOLT`.

### 4.3 `app/boards/blackpill_f401cc.overlay` — Devicetree (which ADC pin)

```dts
/ {
    zephyr,user {
        io-channels = <&adc1 1>;
    };
};
&adc1 {
    #address-cells = <1>;
    #size-cells = <0>;
    channel@1 {
        reg = <1>;
        zephyr,gain = "ADC_GAIN_1";
        zephyr,reference = "ADC_REF_INTERNAL";
        zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
        zephyr,resolution = <12>;
    };
};
```

Same pattern as `zephyr/samples/drivers/adc/adc_dt` and
`zephyr/tests/drivers/adc/adc_api/boards/nucleo_f401re.overlay`:

* `zephyr,user.io-channels` — the list the app iterates with `ADC_DT_SPEC_GET_BY_IDX`.
* `&adc1` — the STM32 ADC instance (PA1 = channel 1 on F401, already pin-muxed in
  the board `.dts` as `adc1_in1_pa1`).
* File name **must** match the board target exactly; it applies only for
  `-b blackpill_f401cc`. `native_sim` builds ignore it (no `adc1` node there).

### 4.4 `app/src/main.c` — both modes in one file

* LED: `DT_ALIAS(led0)` → `GPIO_DT_SPEC_GET` → `gpio_pin_configure_dt` → `drive_led()`
  turns the LED on when `digital_to_volt(raw)` exceeds `LED_THRESHOLD_V` (`VREF/2000`).
* Real ADC path (overlay with `io-channels` present, e.g. `-b blackpill_f401cc`):
  `adc_is_ready_dt` → `adc_channel_setup_dt` → loop `adc_sequence_init_dt` +
  `adc_read_dt` → `digital_to_volt(raw)` → `LOG_INF("raw=%u -> %.3f V")`.
* Simulate path (no `io-channels`, e.g. `native_sim`): sweeps `raw = 0, step,
  2*step ... max` with `step = max/10`, converts and logs identically — no ADC
  driver touched, so it runs on `native_sim`/`QEMU`/any board.
* Mode is chosen at **build time** from Devicetree (`#if` on `io-channels`), not at
  run time — the unused branch is not in the binary.

---

## 5. Build and run — native_sim (simulate, no hardware)

```bash
source ~/zephyrproject/.venv/bin/activate

cd "ITI-Zephyr-Playground/06_Create_Custom_Module_(Digital_To_Volt)/app"

# pristine build, module found via ZEPHYR_EXTRA_MODULES:
west build -p always -b native_sim -- -DZEPHYR_EXTRA_MODULES=../digital_to_volt_Module .

# run (the executable runs on your PC):
west build -t run
```

Expected output (sweep 0 → 4095, LED logic runs but there is no physical LED):

```
*** Booting Zephyr OS build ... ***
[00000000] <inf> d2v_demo: D2V demo: SIMULATE mode
[00000500] <inf> d2v_demo: raw=0 -> 0.000 V
[00001000] <inf> d2v_demo: raw=410 -> 112750.000 V
...
```

Prove Kconfig is build-time: change Vref without touching C code:

```bash
west build -t menuconfig
# → Digital to Volt → Reference voltage in mV → set 2500 → Save → Quit
west build
west build -t run   # millivolt values scale to the new Vref
```

Inspect the resolved config:

```bash
grep DIGITAL_TO_VOLT build/.config
grep DIGITAL_TO_VOLT build/zephyr/include/generated/autoconf.h
```

---

## 6. Build and flash — Black Pill (real ADC)

Wire PA1 to a test voltage (potentiometer wiper, 0–3.3 V; **never exceed 3.3 V**).
GND must be common.

```bash
source ~/zephyrproject/.venv/bin/activate

cd "ITI-Zephyr-Playground/06_Create_Custom_Module_(Digital_To_Volt)/app"

# Build for the Black Pill, same extra-module flag.
# The boards/blackpill_f401cc.overlay provides io-channels, so the app
# automatically takes the real-ADC path — no prj.conf change needed.
west build -p always -b blackpill_f401cc -- -DZEPHYR_EXTRA_MODULES=../digital_to_volt_Module .

# Flash via ROM DFU (no ST-Link needed):
#    hold BOOT0, tap NRST, release BOOT0 → lsusb shows 0483:df11
west flash
#    tap NRST once to boot
```

Watch the console — Black Pill USB-C is **power + DFU only**, so attach a USB-TTL
dongle: PA9 → RX, PA10 → TX, GND → GND, 115200 8N1:

```bash
python -m serial.tools.miniterm /dev/ttyUSB0 115200
```

Expected:

```
*** Booting Zephyr OS build ... ***
[00000000] <inf> d2v_demo: D2V demo: real ADC mode
[00000500] <inf> d2v_demo: raw=6 -> 1650.000 V
```

Turn the pot: LED (PC13) turns **on when the converted value exceeds `VREF/2000`**
(1.65 at 3.3 V Vref), off below. That threshold is `LED_THRESHOLD_V` in `main.c` —
it follows `VREF_MV` automatically; change Vref in menuconfig, rebuild, reflash.

---

## 7. How it works — Kconfig + CMake + Devicetree in one build

```
prj.conf (CONFIG_ADC=y, CONFIG_DIGITAL_TO_VOLT_*) ──┐
digital_to_volt_Module/Kconfig (defines the 3 symbols)           ├─► kconfiglib ─► build/.config ─► autoconf.h
board defconfig                                       │                    (#define CONFIG_DIGITAL_TO_VOLT_VREF_MV 3300 ...)
                                                      │
boards/blackpill_f401cc.overlay ─────────────────────┼─► dtc/gen_defines ─► devicetree_generated.h
board .dts + SoC .dtsi                               │                    (DT_ALIAS(led0) -> &gpioc 13 ...)
                                                     ▼
digital_to_volt_Module/CMakeLists.txt (if(CONFIG_DIGITAL_TO_VOLT)) ─► libdigital_to_volt.a
app/CMakeLists.txt + src/main.c (#if on io-channels) ──► zephyr.elf/.bin
```

* **Kconfig = what code exists + which constants** (`CONFIG_DIGITAL_TO_VOLT=n`
  removes the library; `VREF_MV` / `RESOLUTION_BITS` become `#define`s).
  See folder `05_KConfig`.
* **CMake = how files become a binary** (the `if(CONFIG_DIGITAL_TO_VOLT)` block gates the
  `.c`; `ZEPHYR_EXTRA_MODULES` adds the module to the build).
* **Devicetree = which hardware** (`led0` = PC13, `io-channels` = ADC1/PA1).
  See folder `04_Kconfig_vs_Devicetree(DTS)`.

---

## 8. Customizing

| Want | Where | How |
|---|---|---|
| Different Vref (e.g. 2.5 V external ref) | `app/prj.conf` | `CONFIG_DIGITAL_TO_VOLT_VREF_MV=2500` |
| 10-bit mode | `app/prj.conf` + overlay | `CONFIG_DIGITAL_TO_VOLT_RESOLUTION_BITS=10`, `zephyr,resolution = <10>` |
| Use PA0 instead of PA1 | `app/boards/blackpill_f401cc.overlay` | `io-channels = <&adc1 0>` + `channel@0 { reg = <0>; ... }` |
| Port to F411CE board | new overlay | copy to `app/boards/blackpill_f411ce.overlay`, `-b blackpill_f411ce` |
| Reuse module in another app | any app | `west build -- -DZEPHYR_EXTRA_MODULES=<path>/digital_to_volt_Module` + `#include <digital_to_volt/digital_to_volt.h>` |
| Unit-test the math on PC | any C compiler | compile `digital_to_volt_Module/src/digital_to_volt.c` standalone — it has no Zephyr includes |

---

## 9. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `digital_to_volt/digital_to_volt.h: No such file` | Module not on the build: forgot `-DZEPHYR_EXTRA_MODULES=../digital_to_volt_Module`, or typo in path. Must be passed at **configure** time (first `west build`), not on rebuilds. |
| `undefined reference to digital_to_volt` | `CONFIG_DIGITAL_TO_VOLT=n` in `.config` (library gated out by the `if()` in `CMakeLists.txt`) — set `=y` in `prj.conf`, pristine rebuild. |
| `CONFIG_DIGITAL_TO_VOLT_* undeclared / menuconfig missing` | `zephyr/module.yml` not found or `kconfig:` line wrong — check filename `zephyr/module.yml` inside the module root. |
| `main.c: "CONFIG_DIGITAL_TO_VOLT must be enabled"` | Module disabled — set `CONFIG_DIGITAL_TO_VOLT=y` in `prj.conf`, pristine rebuild. |
| `ADC device not ready / channel setup failed` | Overlay not applied (wrong board name in filename?), `CONFIG_ADC=n`, or PA1 shorted/over-voltage. Check `build/zephyr/zephyr.dts` contains `io-channels`. |
| `west flash: No DFU capable device` | Redo BOOT0+NRST dance; `lsusb \| grep df11`; `dfu-util -l`. |
| Stale Kconfig after editing `prj.conf` | `west build -p always ...` (pristine) or delete `build/`. Never hand-edit `build/.config`. |

---

## 10. Cheat sheet

```bash
# SIMULATE on PC (no hardware — no io-channels on native_sim)
west build -p always -b native_sim -- -DZEPHYR_EXTRA_MODULES=../digital_to_volt_Module .
west build -t run

# REAL ADC on Black Pill (overlay provides io-channels → real path)
west build -p always -b blackpill_f401cc -- -DZEPHYR_EXTRA_MODULES=../digital_to_volt_Module .
west flash   # DFU mode first: hold BOOT0, tap NRST, release BOOT0

# inspect
grep DIGITAL_TO_VOLT build/.config
grep DIGITAL_TO_VOLT build/zephyr/include/generated/autoconf.h
cat build/zephyr/zephyr.dts | grep -A3 io-channels
west build -t menuconfig   # Digital to Volt menu
```

Related in this playground: `05_KConfig` (the whole configuration system),
`04_Kconfig_vs_Devicetree(DTS)` (which half does what),
`03_ Create_New_Project` (minimal app this builds on).

*Zephyr v4.4.0-rc1 — Ubuntu host — Black Pill blackpill_f401cc + native_sim.*
