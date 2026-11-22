# 08 — Create a Custom Devicetree

> **Goal:** define your own Devicetree binding and use it in an application —
> the Devicetree equivalent of creating a custom module (see
> [06 Create Custom Module](<../06_Create_Custom_Module_(Digital_To_Volt)/README.md>)).
>
> **Prerequisites:** folders `03`–`07` in this playground (project creation,
> Kconfig vs Devicetree, Kconfig deep dive, Devicetree deep dive). Every
> command assumes:
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
> - [Devicetree Overlays Guide](<../07_Devicetree/Devicetree_Overlays_Guide.md>)
>   — where to find binding properties and how to write overlays.
> - [Kconfig deep dive](../05_KConfig/README.md) — the configuration system.

---

## Table of Contents

1. [What you are building](#1-what-you-are-building)
2. [Two cases: where to put the custom Devicetree](#2-two-cases-where-to-put-the-custom-devicetree)
3. [Workspace layout (all files)](#3-workspace-layout-all-files)
4. [The custom binding, file by file](#4-the-custom-binding-file-by-file)
5. [The demo app, file by file](#5-the-demo-app-file-by-file)
6. [Build and run — Case A (outside Zephyr, auto-discovered)](#6-build-and-run--case-a-outside-zephyr-auto-discovered)
7. [Build and run — Case B (separate DTS root, inside or outside Zephyr)](#7-build-and-run--case-b-separate-dts-root-inside-or-outside-zephyr)
8. [How it works — DTS_ROOT, bindings, overlays](#8-how-it-works--dts_root-bindings-overlays)
9. [Customizing](#9-customizing)
10. [Troubleshooting](#10-troubleshooting)
11. [Cheat sheet](#11-cheat-sheet)

---

## 1. What you are building

A **custom Devicetree binding** plus a demo app that uses it:

- A binding file `power-switch.yaml` with `compatible: "power-switch"`.
- An application that references a `load_switch` node and drives it with the
  GPIO API, exactly like the Zephyr sample `samples/basic/custom_dts_binding`.
- The same example built in two different layouts (Case A and Case B).

Conversion of the idea: a GPIO pin used for a non-standard purpose (here, a
MOSFET load switch) is described in Devicetree with a custom `compatible`,
instead of reusing `gpio-leds`.

---

## 2. Two cases: where to put the custom Devicetree

Zephyr discovers Devicetree files, bindings, and vendor prefixes through
**DTS roots**. The application directory, the board directory, and
`ZEPHYR_BASE` are always DTS roots. You can add more.

### Case A — Outside the Zephyr workspace (next to the app)

Put the binding inside the application folder:

```text
app/dts/bindings/power-switch.yaml
```

Because the application directory is always a DTS root, Zephyr finds the binding
automatically. **No extra build flag is needed.** This is the simplest option
and is what the Zephyr sample does.

### Case B — Inside or outside the Zephyr workspace (separate DTS root)

Keep the binding in its own folder (a "DTS root") and point Zephyr at it:

```text
custom_dts/dts/bindings/power-switch.yaml
```

Build with:

```bash
west build ... -- -DDTS_ROOT=/path/to/custom_dts
```

This is the right choice when:

- the binding is shared by several applications,
- you want to version it separately from any one app,
- you want to place it inside the Zephyr workspace (e.g.
  `~/zephyrproject/custom_dts`) and reuse it from many apps.

### Inside vs outside the Zephyr workspace

| Location | Path example | When to use |
|---|---|---|
| **Outside** (next to the app) | `08/.../app/dts/bindings/` | One project, simple, no extra flags |
| **Outside** (separate folder) | `08/.../custom_dts/` | Shared by several apps in one repo |
| **Inside** (Zephyr workspace) | `~/zephyrproject/custom_dts/` | Shared by many projects, stable location |

The build command is the same in all "separate DTS root" cases — only the path
changes. You can even list several roots (semicolon-separated).

> ⚠️ **Case A and Case B are alternatives.** Do not define the same
> `compatible` in two active DTS roots at once — the build fails with a
> "duplicate binding" error. Pick one.

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
│   ├── dts/bindings/                    ← Case A: auto-discovered binding
│   │   └── power-switch.yaml
│   └── src/
│       └── main.c
└── custom_dts/                          ← Case B: separate DTS root
    └── dts/bindings/
        └── power-switch.yaml
```

---

## 4. The custom binding, file by file

### `dts/bindings/power-switch.yaml`

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

What this binding does:

- `compatible: "power-switch"` — the string a node must use to match this
  binding. (No vendor prefix here; for real hardware, prefer a vendor prefix
  and add it to a `vendor-prefixes.txt` in the same DTS root.)
- `gpios` is the only property, and it is **required**.
- `type: phandle-array` means it must be a `<&controller pin flags>` tuple.
- The `examples` block shows valid usage and is checked by Zephyr's binding
  tests.

---

## 5. The demo app, file by file

### 5.1 `app/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(custom_dts_demo)
target_sources(app PRIVATE src/main.c)
```

For Case B you can also set the DTS root here instead of on the command line:

```cmake
set(DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../custom_dts)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
```

### 5.2 `app/prj.conf`

```conf
CONFIG_GPIO=y
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

This creates the node that the application references via `DT_NODELABEL(load_switch)`.

### 5.4 `app/src/main.c`

```c
#include <stdio.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#if !DT_NODE_EXISTS(DT_NODELABEL(load_switch))
#error "Overlay for the power-switch node is not defined."
#endif

static const struct gpio_dt_spec load_switch =
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(load_switch), gpios, {0});

int main(void)
{
	int err;

	if (!gpio_is_ready_dt(&load_switch)) {
		printf("The load-switch GPIO port is not ready.\n");
		return 0;
	}

	printf("Initializing load switch (inactive).\n");

	err = gpio_pin_configure_dt(&load_switch, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		printf("Configuring GPIO pin failed: %d\n", err);
		return 0;
	}

	printf("Waiting one second.\n");
	k_sleep(K_MSEC(1000));

	printf("Setting load switch to active level.\n");
	err = gpio_pin_set_dt(&load_switch, 1);
	if (err != 0) {
		printf("Setting GPIO level failed: %d\n", err);
	}
	return 0;
}
```

Key points:

- `DT_NODE_EXISTS(DT_NODELABEL(load_switch))` fails the build early if the
  overlay is missing.
- `GPIO_DT_SPEC_GET_OR(..., {0})` extracts the controller, pin, and flags.
- The binding is custom, but the API is the standard GPIO API — no driver
  code needed.

---

## 6. Build and run — Case A (outside Zephyr, auto-discovered)

The binding lives in `app/dts/bindings/`, which is auto-discovered.

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject

west build -p always -b blackpill_f401cc \
  -d build_custom_a \
  /media/gemy/Linux_Workspace/ITI/Zypher_WS/ITI-Zephyr-Playground/08_Create_Custom_Devicetree/app

west flash -d build_custom_a
```

No extra flags. The build log shows the binding being picked up from the app
directory.

---

## 7. Build and run — Case B (separate DTS root, inside or outside Zephyr)

### 7.1 Outside the Zephyr workspace (separate folder)

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject

# Remove or rename app/dts/bindings/power-switch.yaml first to avoid
# the "duplicate binding" error, then:
west build -p always -b blackpill_f401cc \
  -d build_custom_b \
  /media/gemy/Linux_Workspace/ITI/Zypher_WS/ITI-Zephyr-Playground/08_Create_Custom_Devicetree/app \
  -- -DDTS_ROOT=/media/gemy/Linux_Workspace/ITI/Zypher_WS/ITI-Zephyr-Playground/08_Create_Custom_Devicetree/custom_dts

west flash -d build_custom_b
```

### 7.2 Inside the Zephyr workspace

Copy the DTS root into the workspace (next to `zephyr/`, not inside it, so
`west update` never touches it):

```bash
cp -r /media/gemy/Linux_Workspace/ITI/Zypher_WS/ITI-Zephyr-Playground/08_Create_Custom_Devicetree/custom_dts \
      ~/zephyrproject/custom_dts
```

Then build from any app, anywhere:

```bash
west build -p always -b blackpill_f401cc \
  -d build_custom_b \
  /path/to/any/app \
  -- -DDTS_ROOT=~/zephyrproject/custom_dts
```

You can also list several roots (semicolon-separated) to combine multiple
custom DTS roots:

```bash
-- -DDTS_ROOT="~/zephyrproject/custom_dts;/another/root"
```

---

## 8. How it works — DTS_ROOT, bindings, overlays

```text
DTS roots (app dir, board dir, ZEPHYR_BASE, + your extras)
        │
        ▼
  Each root contributes:
    dts/bindings/*.yaml   → binding schemas
    dts/common/, dts/vendor/, dts/<arch>/, dts/  → #include search paths
    include/, include/zephyr/  → C headers
        │
        ▼
  gen_defines.py reads the board .dts + overlays + bindings
        │
        ▼
  build/zephyr/zephyr.dts
  build/zephyr/include/generated/zephyr/devicetree_generated.h
        │
        ▼
  DT_NODELABEL(load_switch), GPIO_DT_SPEC_GET_OR(...)
```

Key points:

- The application directory is **always** a DTS root, so `app/dts/bindings/`
  is auto-discovered (Case A).
- `boards/<board>.overlay` is also auto-discovered because the app dir is a
  root and contains `boards/`.
- Bindings are matched by `compatible` string → file name
  (`power-switch` → `power-switch.yaml`).
- A node without a matching binding makes the build fail — the binding is the
  contract.

---

## 9. Customizing

| Want | Where | How |
|---|---|---|
| Add a property to the binding | `dts/bindings/power-switch.yaml` | Add under `properties:` with type and `required` |
| Use a vendor prefix | binding + `vendor-prefixes.txt` | `compatible: "mycompany,power-switch"` + add prefix to `dts/bindings/vendor-prefixes.txt` |
| Add a child node | binding | Use `child-binding:` like `gpio-leds` does |
| Include a default value | binding | Add `default:` (and justify it in `description`) |
| Reuse in another app | any app | `west build ... -- -DDTS_ROOT=/path/to/custom_dts` |
| Put the root inside Zephyr | `~/zephyrproject/custom_dts/` | Copy the folder, reuse the path from any app |

---

## 10. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `power-switch.yaml: No such file or directory` / unknown compatible | Binding not on any DTS root. For Case A, check `app/dts/bindings/`. For Case B, pass `-DDTS_ROOT`. |
| `both ... and ... have 'compatible: power-switch'` | Duplicate binding across two active roots. Use only one case at a time. |
| `DT_NODELABEL(load_switch) not found` / `#error` in main.c | Overlay not applied. Check the overlay filename matches the board target exactly. |
| `GPIO device not ready` at runtime | The GPIO controller is not enabled. Check `status = "okay"` on `&gpioa` and `CONFIG_GPIO=y`. |
| Binding changes not picked up | Pristine rebuild: `west build -p always`. Bindings are resolved at configure time. |
| `vendor-prefixes.txt` warning | Add the prefix to a `dts/bindings/vendor-prefixes.txt` in your DTS root. |

---

## 11. Cheat sheet

```bash
# Case A — auto-discovered (binding in app/dts/bindings/)
west build -p always -b <board> -d build_app <app>

# Case B — separate DTS root (outside Zephyr)
west build -p always -b <board> -d build_app <app> \
  -- -DDTS_ROOT=/path/to/custom_dts

# Case B — inside Zephyr workspace
cp -r custom_dts ~/zephyrproject/custom_dts
west build -p always -b <board> -d build_app <app> \
  -- -DDTS_ROOT=~/zephyrproject/custom_dts

# Inspect
grep -n "load_switch\|power-switch" build/zephyr/zephyr.dts
grep -rn "power-switch" build/zephyr/include/generated/zephyr/devicetree_generated.h
cat build/zephyr/.config | grep GPIO

# Pristine rebuild after binding changes
west build -p always -b <board> -d build_app <app>
```

---

## Related documents

- [Devicetree deep dive](../07_Devicetree/README.md) — concepts, syntax,
  bindings, macros.
- [Devicetree Overlays Guide](<../07_Devicetree/Devicetree_Overlays_Guide.md>)
  — where to find overlay properties and how to write overlays.
- [Create a New Zephyr Project](../03_Create_New_Project/README.md) — full
  project examples.
- [Kconfig vs Devicetree](<../04_Kconfig_vs_Devicetree(DTS)/README.md>) —
  which half does what.
- [Kconfig deep dive](../05_KConfig/README.md) — the configuration system.
- [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md)
  — how to find the right `CONFIG_...` option.
- [Using Drivers and Software Modules](../03_Create_New_Project/Using_Drivers_And_Software_Modules.md)
  — the general lifecycle for using any driver, subsystem, or module.
- [Custom Module](<../06_Create_Custom_Module_(Digital_To_Volt)/README.md>) —
  out-of-tree modules with `ZEPHYR_EXTRA_MODULES`.

*Zephyr v4.4.0-rc1 — Ubuntu host — Black Pill blackpill_f401cc.*
