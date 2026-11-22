# 08 — Create a Custom Devicetree (with code)

> **Goal:** define your own Devicetree binding **and** back it with a small
> helper library — the Devicetree equivalent of creating a custom module (see
> [06 Create Custom Module](<../06_Create_Custom_Module_(Digital_To_Volt)/README.md>)).
> The `custom_dts/` folder is both a **Zephyr module** (code) and a **DTS root**
> (binding), so one flag (`ZEPHYR_EXTRA_MODULES`) provides both.
>
> **Prerequisites:** folders `03`–`07` in this playground. Every command assumes:
> ```bash
> source ~/zephyrproject/.venv/bin/activate
> ```
>
> **Learning path:** `03 Create Project` → `04 Kconfig vs Devicetree` →
> `05 Kconfig deep dive` → `07 Devicetree` → **`08 Create Custom Devicetree`**.
>
> **Companion guides:**
> - [Devicetree deep dive](../07_Devicetree/README.md) — concepts, syntax,
>   bindings, macros.
> - [Devicetree Overlays Guide](../07_Devicetree/Devicetree_Overlays_Guide.md)
>   — where to find overlay properties and how to write overlays.
> - [Custom Module](<../06_Create_Custom_Module_(Digital_To_Volt)/README.md>) —
>   out-of-tree modules with `ZEPHYR_EXTRA_MODULES`.
> - [Kconfig deep dive](../05_KConfig/README.md) — the configuration system.

---

## Table of Contents

1. [What you are building](#1-what-you-are-building)
2. [Two cases: where to put the custom Devicetree + code](#2-two-cases-where-to-put-the-custom-devicetree--code)
3. [Workspace layout (all files)](#3-workspace-layout-all-files)
4. [The custom module + binding, file by file](#4-the-custom-module--binding-file-by-file)
5. [The demo app, file by file](#5-the-demo-app-file-by-file)
6. [Build and run — Case A: outside the Zephyr workspace](#6-build-and-run--case-a-outside-the-zephyr-workspace)
7. [Build and run — Case B: inside the Zephyr workspace](#7-build-and-run--case-b-inside-the-zephyr-workspace)
8. [How it works — module + DTS root](#8-how-it-works--module--dts-root)
9. [Lighter alternative: binding only (no code)](#9-lighter-alternative-binding-only-no-code)
10. [Customizing](#10-customizing)
11. [Troubleshooting](#11-troubleshooting)
12. [Cheat sheet](#12-cheat-sheet)

---

## 1. What you are building

A **custom Devicetree binding** plus a small helper library that uses it:

- A binding file `power-switch.yaml` with `compatible: "power-switch"`.
- A module `power_switch` (Kconfig + CMakeLists + `src/` + `include/`) that
  wraps the GPIO driving logic.
- A demo app that references a `load_switch` node and drives it through the
  helper, exactly like the Zephyr sample `samples/basic/custom_dts_binding`,
  but with the code moved into a reusable module.

The `custom_dts/` folder is **both** a module (code) and a DTS root
(binding), so a single `-DZEPHYR_EXTRA_MODULES=...` flag provides everything.

---

## 2. Two cases: where to put the custom Devicetree + code

The module (and its DTS root) can live anywhere. The build command is the same
in both cases — only the path changes. You can even list several modules
(semicolon-separated).

### Case A — Outside the Zephyr workspace (next to the app, or any folder)

```text
08_Create_Custom_Devicetree/custom_dts/   ← module + DTS root
```

Build with:

```bash
west build ... -- -DZEPHYR_EXTRA_MODULES=/path/to/custom_dts
```

Use this when the module is versioned with one project or shared by a few apps
in one repo.

### Case B — Inside the Zephyr workspace

Copy the module into the workspace (next to `zephyr/`, **not** inside
`zephyr/`, so `west update` never touches it):

```bash
cp -r custom_dts ~/zephyrproject/custom_dts
```

Then build from any app, anywhere:

```bash
west build ... -- -DZEPHYR_EXTRA_MODULES=~/zephyrproject/custom_dts
```

Use this when the module is shared by many projects.

> ⚠️ **Do not define the same `compatible` in two active modules/roots at
> once** — the build fails with a "duplicate binding" error. Pick one.

---

## 3. Workspace layout (all files)

```text
08_Create_Custom_Devicetree/
├── README.md
├── app/                                 ← demo application
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── boards/
│   │   └── blackpill_f401cc.overlay     ← creates the load_switch node
│   └── src/
│       └── main.c
└── custom_dts/                          ← module + DTS root (code + binding)
    ├── zephyr/module.yml                ← module name, cmake, kconfig, dts_root
    ├── CMakeLists.txt                   ← zephyr_library() gated on Kconfig
    ├── Kconfig                          ← CONFIG_POWER_SWITCH + log level
    ├── include/power_switch/
    │   └── power_switch.h               ← public API
    ├── src/
    │   └── power_switch.c               ← implementation
    └── dts/bindings/
        └── power-switch.yaml            ← the custom binding
```

---

## 4. The custom module + binding, file by file

### 4.1 `custom_dts/zephyr/module.yml`

```yaml
name: power_switch
build:
  cmake: .
  kconfig: Kconfig
  settings:
    dts_root: .
```

- `name` — module identifier shown in CMake logs.
- `build.cmake` / `build.kconfig` — where the module's build files are.
- `build.settings.dts_root: .` — **this module also contributes a DTS root**,
  so its `dts/bindings/` and `dts/` folders are discovered automatically.
  One flag gives you both code and the binding.

### 4.2 `custom_dts/CMakeLists.txt`

```cmake
if(CONFIG_POWER_SWITCH)

    zephyr_library()

    zephyr_library_sources(
        src/power_switch.c
    )

    zephyr_include_directories(
        include
    )

endif()
```

The whole library is compiled only when `CONFIG_POWER_SWITCH=y`. With
`=n`, the module contributes zero bytes.

### 4.3 `custom_dts/Kconfig`

```kconfig
menuconfig POWER_SWITCH
    bool "Power switch helper"
    default y
    help
      Enable the power-switch helper library.

if POWER_SWITCH

config POWER_SWITCH_LOG_LEVEL
    int "Log level for power-switch helper"
    default 3
    range 0 4
    help
      0=NONE 1=ERR 2=WRN 3=INF 4=DBG.

endif
```

### 4.4 `custom_dts/include/power_switch/power_switch.h`

```c
#ifndef POWER_SWITCH_H
#define POWER_SWITCH_H

#include <zephyr/drivers/gpio.h>

struct power_switch {
	struct gpio_dt_spec gpio;
};

int power_switch_init(struct power_switch *sw, const struct gpio_dt_spec *spec);
int power_switch_set(struct power_switch *sw, int value);

#endif
```

### 4.5 `custom_dts/src/power_switch.c`

```c
#include <power_switch/power_switch.h>

#include <errno.h>

int power_switch_init(struct power_switch *sw, const struct gpio_dt_spec *spec)
{
	int err;

	if (sw == NULL || spec == NULL) {
		return -EINVAL;
	}

	sw->gpio = *spec;

	if (!gpio_is_ready_dt(&sw->gpio)) {
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&sw->gpio, GPIO_OUTPUT_INACTIVE);
	return err;
}

int power_switch_set(struct power_switch *sw, int value)
{
	if (sw == NULL) {
		return -EINVAL;
	}
	return gpio_pin_set_dt(&sw->gpio, value);
}
```

### 4.6 `custom_dts/dts/bindings/power-switch.yaml`

```yaml
description: GPIO pin to switch a power output on or off

compatible: "power-switch"

properties:
  gpios:
    type: phandle-array
    required: true
    description: |
      The GPIO connected to the gate driver for the MOSFET / load switch.

examples:
  - |
    / {
      load_switch: load_switch {
        compatible = "power-switch";
        gpios = <&gpioa 5 GPIO_ACTIVE_HIGH>;
      };
    };
```

---

## 5. The demo app, file by file

### 5.1 `app/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(custom_dts_demo)
target_sources(app PRIVATE src/main.c)
```

The module is **not** copied here — it is found via `ZEPHYR_EXTRA_MODULES` on
the `west build` command line (Sections 6/7).

### 5.2 `app/prj.conf`

```conf
CONFIG_GPIO=y
CONFIG_POWER_SWITCH=y
```

### 5.3 `app/boards/blackpill_f401cc.overlay`

```dts
/ {
	load_switch: load_switch {
		compatible = "power-switch";
		gpios = <&gpioa 5 GPIO_ACTIVE_HIGH>;
	};
};
```

### 5.4 `app/src/main.c`

```c
#include <stdio.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include <power_switch/power_switch.h>

#if !DT_NODE_EXISTS(DT_NODELABEL(load_switch))
#error "Overlay for the power-switch node is not defined."
#endif

static const struct gpio_dt_spec load_switch_spec =
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(load_switch), gpios, {0});

static struct power_switch load_switch;

int main(void)
{
	int err;

	err = power_switch_init(&load_switch, &load_switch_spec);
	if (err != 0) {
		printf("Initializing power switch failed: %d\n", err);
		return 0;
	}

	printf("Waiting one second.\n");
	k_sleep(K_MSEC(1000));

	printf("Setting load switch to active level.\n");
	err = power_switch_set(&load_switch, 1);
	if (err != 0) {
		printf("Setting load switch level failed: %d\n", err);
	}
	return 0;
}
```

---

## 6. Build and run — Case A: outside the Zephyr workspace

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject

west build -p always -b blackpill_f401cc \
  -d build_custom_a \
  /media/gemy/Linux_Workspace/ITI/Zypher_WS/ITI-Zephyr-Playground/08_Create_Custom_Devicetree/app \
  -- -DZEPHYR_EXTRA_MODULES=/media/gemy/Linux_Workspace/ITI/Zypher_WS/ITI-Zephyr-Playground/08_Create_Custom_Devicetree/custom_dts

west flash -d build_custom_a
```

One flag provides both the helper library and the binding.

---

## 7. Build and run — Case B: inside the Zephyr workspace

Copy the module into the workspace (next to `zephyr/`, not inside it):

```bash
cp -r /media/gemy/Linux_Workspace/ITI/Zypher_WS/ITI-Zephyr-Playground/08_Create_Custom_Devicetree/custom_dts \
      ~/zephyrproject/custom_dts
```

Then build from any app, anywhere:

```bash
west build -p always -b blackpill_f401cc \
  -d build_custom_b \
  /path/to/any/app \
  -- -DZEPHYR_EXTRA_MODULES=~/zephyrproject/custom_dts

west flash -d build_custom_b
```

You can also list several modules (semicolon-separated).

---

## 8. How it works — module + DTS root

```text
ZEPHYR_EXTRA_MODULES=custom_dts
        │
        ├──► zephyr_module.py reads zephyr/module.yml
        │       ├── build.cmake  → CMakeLists.txt → zephyr_library() + src/power_switch.c
        │       ├── build.kconfig → Kconfig → CONFIG_POWER_SWITCH
        │       └── build.settings.dts_root: . → dts/bindings/power-switch.yaml
        │
        ▼
  Binding matched by compatible "power-switch"
  Overlay creates load_switch node
        │
        ▼
  gen_defines.py → devicetree_generated.h (DT_NODELABEL(load_switch))
        │
        ▼
  app links power_switch library → zephyr.elf
```

Key points:

- `module.yml` with `dts_root` makes the module **also** a DTS root, so the
  binding is discovered automatically — no separate `-DDTS_ROOT` needed.
- The binding is matched by `compatible` string → file name
  (`power-switch` → `power-switch.yaml`).
- With `CONFIG_POWER_SWITCH=n`, the library contributes zero bytes.

---

## 9. Lighter alternative: binding only (no code)

If you do not need a helper library and just want to describe a GPIO with a
custom `compatible`, put the binding in the application folder:

```text
app/dts/bindings/power-switch.yaml
```

Because the application directory is always a DTS root, Zephyr finds it
automatically — **no extra build flag**. The app then uses the standard GPIO
API directly (see the original `samples/basic/custom_dts_binding`).

> ⚠️ **Do not define the same `compatible` in two active roots at once** —
> the build fails with a "duplicate binding" error.

---

## 10. Customizing

| Want | Where | How |
|---|---|---|
| Add a property to the binding | `custom_dts/dts/bindings/power-switch.yaml` | Add under `properties:` |
| Use a vendor prefix | binding + `vendor-prefixes.txt` | `compatible: "mycompany,power-switch"` + add prefix to `custom_dts/dts/bindings/vendor-prefixes.txt` |
| Extend the helper API | `custom_dts/include/...` + `src/...` | Add functions, rebuild |
| Reuse in another app | any app | `west build ... -- -DZEPHYR_EXTRA_MODULES=/path/to/custom_dts` |
| Put the module inside Zephyr | `~/zephyrproject/custom_dts/` | Copy the folder, reuse the path |

---

## 11. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `power-switch.yaml: No such file` / unknown compatible | Module not on the build: forgot `-DZEPHYR_EXTRA_MODULES`, or typo in path. Must be passed at **configure** time. |
| `both ... and ... have 'compatible: power-switch'` | Duplicate binding across two active roots. Use one module at a time. |
| `DT_NODELABEL(load_switch) not found` / `#error` | Overlay not applied. Check the overlay filename matches the board target exactly. |
| `GPIO device not ready` at runtime | The GPIO controller is not enabled. Check `status = "okay"` on `&gpioa` and `CONFIG_GPIO=y`. |
| `power_switch/power_switch.h: No such file` | Module not found — pass `-DZEPHYR_EXTRA_MODULES`. |
| Binding/code changes not picked up | Pristine rebuild: `west build -p always`. |

---

## 12. Cheat sheet

```bash
# Case A — module next to the app (outside Zephyr)
west build -p always -b blackpill_f401cc \
  -d build_custom /path/to/app \
  -- -DZEPHYR_EXTRA_MODULES=/path/to/custom_dts

# Case B — module inside the Zephyr workspace
cp -r custom_dts ~/zephyrproject/custom_dts
west build -p always -b blackpill_f401cc \
  -d build_custom /path/to/app \
  -- -DZEPHYR_EXTRA_MODULES=~/zephyrproject/custom_dts

# Inspect
grep -n "load_switch\|power-switch" build/zephyr/zephyr.dts
grep -rn "power-switch" build/zephyr/include/generated/zephyr/devicetree_generated.h

# Pristine rebuild after changes
west build -p always -b blackpill_f401cc -d build_custom /path/to/app \
  -- -DZEPHYR_EXTRA_MODULES=/path/to/custom_dts
```

---

## Related documents

- [Devicetree deep dive](../07_Devicetree/README.md) — concepts, syntax,
  bindings, macros.
- [Devicetree Overlays Guide](../07_Devicetree/Devicetree_Overlays_Guide.md)
  — where to find overlay properties and how to write overlays.
- [Custom Module](<../06_Create_Custom_Module_(Digital_To_Volt)/README.md>) —
  out-of-tree modules with `ZEPHYR_EXTRA_MODULES`.
- [Kconfig deep dive](../05_KConfig/README.md) — the configuration system.
- [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md)
  — how to find the right `CONFIG_...` option.
- [Using Drivers and Software Modules](../03_Create_New_Project/Using_Drivers_And_Software_Modules.md)
  — the general lifecycle for using any driver, subsystem, or module.
- [Create a New Zephyr Project](../03_Create_New_Project/README.md) — full
  project examples.

*Zephyr v4.4.0-rc1 — Ubuntu host — Black Pill blackpill_f401cc.*
