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
  `d2v_raw_to_mv()` — the same way it would use any upstream Zephyr library.

Conversion math (integer, no float needed for mV):

```
mv = raw * vref_mv / (2^resolution_bits - 1)
```

Examples at 12-bit / 3.3 V: `0 -> 0 mV`, `2048 -> ~1650 mV`, `4095 -> 3300 mV`.

---

## 2. Workspace layout (all files)

```
06_Create_Custom_Module_(Digital_To_Volt)/
├── README.md                        ← this file (full guide)
├── d2v_module/                      ← THE MODULE (reusable, board-independent)
│   ├── zephyr/module.yml            ← tells Zephyr "this folder is a module"
│   ├── CMakeLists.txt               ← which .c files to compile, where headers are
│   ├── Kconfig                      ← CONFIG_DIGITAL_TO_VOLT, CONFIG_D2V_* options
│   ├── include/digital_to_volt/
│   │   └── digital_to_volt.h        ← public API (d2v_raw_to_mv, ...)
│   └── src/
│       └── digital_to_volt.c        ← implementation (64-bit-safe integer math)
└── app/                             ← DEMO APP (uses the module)
    ├── CMakeLists.txt               ← find_package(Zephyr) + src/main.c
    ├── prj.conf                     ← Kconfig values (GPIO, ADC, LOG, D2V_*)
    ├── boards/
    │   └── blackpill_f401cc.overlay ← ADC1 channel 1 (PA1) -> io-channels
    └── src/
        └── main.c                   ← simulate sweep OR real ADC read + LED by threshold
```

---

## 3. The module, file by file

### 3.1 `d2v_module/zephyr/module.yml`

```yaml
name: digital_to_volt
build:
  cmake: .
  kconfig: Kconfig
```

* `name` — module identifier shown in CMake logs.
* `build.cmake: .` — "the module's CMake file is the `CMakeLists.txt` next to me".
* `build.kconfig: Kconfig` — "parse my `Kconfig` so `CONFIG_D2V_*` exist in menuconfig".

Without this file, `ZEPHYR_EXTRA_MODULES` silently ignores the folder.

### 3.2 `d2v_module/CMakeLists.txt`

```cmake
zephyr_library_named(digital_to_volt)
zephyr_library_sources_ifdef(CONFIG_DIGITAL_TO_VOLT src/digital_to_volt.c)
zephyr_include_directories(include)
```

| Line | Meaning |
|---|---|
| `zephyr_library_named(...)` | Create a static library target for this module. |
| `..._ifdef(CONFIG_DIGITAL_TO_VOLT ...)` | Compile the `.c` **only** when Kconfig enables it (`=n` → zero bytes in binary). |
| `zephyr_include_directories(include)` | Put `include/` on the compiler path so apps can `#include <digital_to_volt/...>`. |

### 3.3 `d2v_module/Kconfig`

```kconfig
menu "Digital To Volt Module"
config DIGITAL_TO_VOLT
    bool "Enable Digital_To_Volt conversion library"
    default y
config D2V_VREF_MV
    int "ADC reference voltage in millivolts"
    depends on DIGITAL_TO_VOLT
    range 100 5000
    default 3300
config D2V_RESOLUTION
    int "ADC resolution in bits"
    depends on DIGITAL_TO_VOLT
    range 6 16
    default 12
config D2V_LED_THRESHOLD_MV
    int "LED threshold in millivolts"
    depends on DIGITAL_TO_VOLT
    range 0 5000
    default 1650
config D2V_SIMULATE
    bool "Simulate ADC sweep instead of reading hardware"
    depends on DIGITAL_TO_VOLT
    default y
endmenu
```

This is **Kconfig** (see folder `05_KConfig` for the full system):

* `config X` defines a symbol → in C it is `CONFIG_X`, in `prj.conf` it is `CONFIG_X=...`.
* `depends on` hides `D2V_*` unless the library itself is on — menuconfig enforces it.
* `range` + `default` give sane Black Pill values (3.3 V, 12-bit, half-scale LED).
* In menuconfig these appear under **Digital To Volt Module**.

### 3.4 `d2v_module/include/digital_to_volt/digital_to_volt.h` + `src/digital_to_volt.c`

Public API:

```c
int32_t d2v_raw_to_mv(uint32_t raw, uint8_t resolution_bits, uint32_t vref_mv);
float   d2v_mv_to_volt(int32_t mv);
float   d2v_raw_to_volt(uint32_t raw, uint8_t resolution_bits, uint32_t vref_mv);
bool    d2v_is_above_threshold_mv(int32_t mv, int32_t threshold_mv);
uint32_t d2v_max_raw(uint8_t resolution_bits);
```

Implementation notes:

* `d2v_max_raw(bits)` = `(1 << bits) - 1` (12 → 4095). Returns 0 on invalid input.
* `d2v_raw_to_mv` clamps over-range codes, then does `raw * vref / max` in
  **64-bit** (`uint64_t`) so `65535 * 5000` never overflows 32-bit.
* The mV path uses **no float** — safe on FPU-less boards and in ISRs.
* `mv_to_volt` / `raw_to_volt` are thin float helpers for `LOG_INF("%.3f V")`.

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
CONFIG_D2V_VREF_MV=3300
CONFIG_D2V_RESOLUTION=12
CONFIG_D2V_LED_THRESHOLD_MV=1650
CONFIG_D2V_SIMULATE=y
```

* First block: Zephyr subsystems (GPIO for LED, ADC for real reads, LOG for output).
* Second block: module options — must match the ADC channel resolution and your
  board's Vref. Set `CONFIG_D2V_SIMULATE=n` on the Black Pill to read PA1.
* Verify after a build: `cat build/.config | grep D2V`, `cat build/zephyr/include/generated/autoconf.h | grep D2V`.

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
  sets the LED by `d2v_is_above_threshold_mv(mv, CONFIG_D2V_LED_THRESHOLD_MV)`.
* Real ADC path (`D2V_USE_REAL_ADC=1`, i.e. `CONFIG_D2V_SIMULATE=0` + overlay):
  `adc_is_ready_dt` → `adc_channel_setup_dt` → loop `adc_sequence_init_dt` +
  `adc_read_dt` → `d2v_raw_to_mv` → `LOG_INF("raw=%u -> %d mV")`.
* Simulate path (default): sweeps `raw = 0, step, 2*step ... max` with
  `step = max/10`, converts and logs identically — no ADC driver touched, so it
  runs on `native_sim`/`QEMU`/any board.
* Mode is chosen at **build time** (`#if`), not at run time — disabled code is
  not in the binary (Kconfig philosophy from folder `05`).

---

## 5. Build and run — native_sim (simulate, no hardware)

```bash
source ~/zephyrproject/.venv/bin/activate

cd "ITI-Zephyr-Playground/06_Create_Custom_Module_(Digital_To_Volt)/app"

# pristine build, module found via ZEPHYR_EXTRA_MODULES:
west build -p always -b native_sim -- -DZEPHYR_EXTRA_MODULES=../d2v_module .

# run (the executable runs on your PC):
west build -t run
```

Expected output (sweep 0 → 4095, LED logic runs but there is no physical LED):

```
*** Booting Zephyr OS build ... ***
[00000000] <inf> d2v_demo: D2V demo: SIMULATE mode, vref=3300 mV, res=12 bit (max=4095)
[00000500] <inf> d2v_demo: raw=0 -> 0 mV (0.000 V)
[00001000] <inf> d2v_demo: raw=409 -> 329 mV (0.329 V)
...
[00005500] <inf> d2v_demo: raw=2045 -> 1648 mV (1.648 V)
[00006000] <inf> d2v_demo: raw=2454 -> 1977 mV (1.977 V)
```

Prove Kconfig is build-time: change the threshold without touching C code:

```bash
west build -t menuconfig
# → Digital To Volt Module → LED threshold → set 1000 → Save → Quit
west build
west build -t run   # LED flips earlier in the sweep
```

Inspect the resolved config:

```bash
grep D2V build/.config
grep D2V build/zephyr/include/generated/autoconf.h
```

---

## 6. Build and flash — Black Pill (real ADC)

Wire PA1 to a test voltage (potentiometer wiper, 0–3.3 V; **never exceed 3.3 V**).
GND must be common.

```bash
source ~/zephyrproject/.venv/bin/activate

cd "ITI-Zephyr-Playground/06_Create_Custom_Module_(Digital_To_Volt)/app"

# 1) Switch the app to real-ADC mode (one-line prj.conf edit):
#    CONFIG_D2V_SIMULATE=n
sed -i 's/^CONFIG_D2V_SIMULATE=y/CONFIG_D2V_SIMULATE=n/' prj.conf
grep D2V_SIMULATE prj.conf

# 2) Build for the Black Pill, same extra-module flag:
west build -p always -b blackpill_f401cc -- -DZEPHYR_EXTRA_MODULES=../d2v_module .

# 3) Flash via ROM DFU (no ST-Link needed):
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
[00000000] <inf> d2v_demo: D2V demo: real ADC mode, vref=3300 mV, res=12 bit
[00000500] <inf> d2v_demo: raw=2048 -> 1650 mV (1.650 V)
```

Turn the pot: LED (PC13) turns **on above 1650 mV**, off below. That threshold is
`CONFIG_D2V_LED_THRESHOLD_MV` — change it in `prj.conf` or menuconfig, rebuild,
reflash.

Back to simulation later:

```bash
sed -i 's/^CONFIG_D2V_SIMULATE=n/CONFIG_D2V_SIMULATE=y/' prj.conf
```

---

## 7. How it works — Kconfig + CMake + Devicetree in one build

```
prj.conf (CONFIG_ADC=y, CONFIG_D2V_*) ──┐
d2v_module/Kconfig (defines D2V_*)      ├─► kconfiglib ─► build/.config ─► autoconf.h
board defconfig                          │                    (#define CONFIG_D2V_VREF_MV 3300 ...)
                                         │
boards/blackpill_f401cc.overlay ─────────┼─► dtc/gen_defines ─► devicetree_generated.h
board .dts + SoC .dtsi                   │                    (DT_ALIAS(led0) -> &gpioc 13 ...)
                                         ▼
d2v_module/CMakeLists.txt (ifdef CONFIG_DIGITAL_TO_VOLT) ─► libdigital_to_volt.a
app/CMakeLists.txt (src/main.c, #if CONFIG_D2V_SIMULATE) ──► zephyr.elf/.bin
```

* **Kconfig = what code exists** (`CONFIG_DIGITAL_TO_VOLT=n` removes the library;
  `CONFIG_D2V_SIMULATE` picks the `#if` branch). See folder `05_KConfig`.
* **CMake = how files become a binary** (`zephyr_library_sources_ifdef` gates the
  `.c`; `ZEPHYR_EXTRA_MODULES` adds the module to the build).
* **Devicetree = which hardware** (`led0` = PC13, `io-channels` = ADC1/PA1).
  See folder `04_Kconfig_vs_Devicetree(DTS)`.

---

## 8. Customizing

| Want | Where | How |
|---|---|---|
| Different Vref (e.g. 2.5 V external ref) | `app/prj.conf` | `CONFIG_D2V_VREF_MV=2500` |
| 10-bit mode | `app/prj.conf` + overlay | `CONFIG_D2V_RESOLUTION=10`, `zephyr,resolution = <10>` |
| LED flips at 2.0 V | `app/prj.conf` | `CONFIG_D2V_LED_THRESHOLD_MV=2000` |
| Use PA0 instead of PA1 | `app/boards/blackpill_f401cc.overlay` | `io-channels = <&adc1 0>` + `channel@0 { reg = <0>; ... }` |
| Port to F411CE board | new overlay | copy to `app/boards/blackpill_f411ce.overlay`, `-b blackpill_f411ce` |
| Reuse module in another app | any app | `west build -- -DZEPHYR_EXTRA_MODULES=<path>/d2v_module` + `#include <digital_to_volt/digital_to_volt.h>` |
| Unit-test the math on PC | any C compiler | compile `d2v_module/src/digital_to_volt.c` standalone — it has no Zephyr includes |

---

## 9. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `digital_to_volt/digital_to_volt.h: No such file` | Module not on the build: forgot `-DZEPHYR_EXTRA_MODULES=../d2v_module`, or typo in path. Must be passed at **configure** time (first `west build`), not on rebuilds. |
| `undefined reference to d2v_raw_to_mv` | `CONFIG_DIGITAL_TO_VOLT=n` in `.config` (library gated out) — set `=y` in `prj.conf`, pristine rebuild. |
| `CONFIG_D2V_* undeclared / menuconfig missing` | `zephyr/module.yml` not found or `kconfig:` line wrong — check filename `zephyr/module.yml` inside the module root. |
| `No suitable io-channels / zephyr_user` error | `CONFIG_D2V_SIMULATE=n` but built for `native_sim` or without the boards overlay — either set SIMULATE=y or build `-b blackpill_f401cc` where the overlay applies. |
| `ADC device not ready / channel setup failed` | Overlay not applied (wrong board name in filename?), `CONFIG_ADC=n`, or PA1 shorted/over-voltage. Check `build/zephyr/zephyr.dts` contains `io-channels`. |
| `west flash: No DFU capable device` | Redo BOOT0+NRST dance; `lsusb \| grep df11`; `dfu-util -l`. |
| Stale Kconfig after editing `prj.conf` | `west build -p always ...` (pristine) or delete `build/`. Never hand-edit `build/.config`. |
| LED never changes on sim | Threshold outside sweep? `grep D2V_LED_THRESHOLD_MV build/.config`. Default 1650 flips mid-sweep. |

---

## 10. Cheat sheet

```bash
# SIMULATE on PC (no hardware)
west build -p always -b native_sim -- -DZEPHYR_EXTRA_MODULES=../d2v_module .
west build -t run

# REAL ADC on Black Pill
sed -i 's/CONFIG_D2V_SIMULATE=y/CONFIG_D2V_SIMULATE=n/' prj.conf
west build -p always -b blackpill_f401cc -- -DZEPHYR_EXTRA_MODULES=../d2v_module .
west flash   # DFU mode first: hold BOOT0, tap NRST, release BOOT0

# inspect
grep D2V build/.config
grep D2V build/zephyr/include/generated/autoconf.h
cat build/zephyr/zephyr.dts | grep -A3 io-channels
west build -t menuconfig   # Digital To Volt Module menu
```

Related in this playground: `05_KConfig` (the whole configuration system),
`04_Kconfig_vs_Devicetree(DTS)` (which half does what),
`03_ Create_New_Project` (minimal app this builds on).

*Zephyr v4.4.0-rc1 — Ubuntu host — Black Pill blackpill_f401cc + native_sim.*
