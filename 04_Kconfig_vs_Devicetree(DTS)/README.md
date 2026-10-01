# Kconfig vs Devicetree (DTS) in Zephyr — Full Comparison Guide

> **The one-sentence answer:** **Kconfig decides *what code is compiled*** (software
> features, at build time). **Devicetree describes *what hardware exists* and how it
> is wired** (pins, buses, addresses). They are two separate systems that meet in
> the build — Kconfig turns features on/off, Devicetree tells the enabled drivers
> which pins and registers to use.

---

## Table of Contents

1. [The 30-second version](#1-the-30-second-version)
2. [Two different questions](#2-two-different-questions)
3. [Side-by-side comparison](#3-side-by-side-comparison)
4. [Kconfig in depth](#4-kconfig-in-depth)
5. [Devicetree in depth](#5-devicetree-in-depth)
6. [How they interact in one build](#6-how-they-interact-in-one-build)
7. [Black Pill example — same project, both systems](#7-black-pill-example--same-project-both-systems)
8. [How to know which one you need](#8-how-to-know-which-one-you-need)
9. [Common confusions](#9-common-confusions)
10. [File reference](#10-file-reference)
11. [Cheat sheet](#11-cheat-sheet)

---

## 1. The 30-second version

| | **Kconfig** | **Devicetree (DTS)** |
|---|---|---|
| Answers | "Is this feature compiled in?" | "Where is the hardware? Which pin?" |
| Domain | **Software** configuration | **Hardware** description |
| Decided | At **build time** | At **build time** (but *describes* hardware) |
| Output | `CONFIG_XXX=y` → `#define` in `autoconf.h` | C macros like `DT_ALIAS(led0)` → `devicetree_generated.h` |
| Controlled by | `prj.conf`, board defconfig, overlays | board `.dts`, `app.overlay` |
| Keyword examples | `config`, `bool`, `depends on`, `select` | `compatible`, `reg`, `gpios`, `status` |
| Typical user edit | `prj.conf` (enable a driver) | `app.overlay` (change a pin) |

> 🔑 **Rule of thumb:** if you're choosing between *including a driver/feature or
> not* → Kconfig. If you're telling a driver *which pin, bus, address, or instance*
> → Devicetree.

---

## 2. Two different questions

Every embedded project must answer two independent questions:

### Question 1 — "What software do I build?" (Kconfig)

- Do I include the GPIO driver? The shell? Bluetooth? The logging subsystem?
- Embedded targets have tiny flash/RAM — unused features must not be compiled at all.
- Kconfig works **only at build time**: enabled = code included, disabled = code
  does not exist in the binary. Zero run-time cost, zero waste.

### Question 2 — "What hardware do I have, and how is it wired?" (Devicetree)

- The LED is on **PC13**, active-low.
- The console UART is **USART1** on pins **PA9/PA10**, 115200 baud.
- There is one I2C controller, on pins **PB8/PB9**.
- Hardware is a fact of the board — it doesn't change at run time, so it is
  described once and compiled into constants.

These answers come from different places (the software stack vs. the board
schematic), are written in different languages, and are consumed by different
parts of the build — but **both end up as compile-time constants** that the
drivers combine.

---

## 3. Side-by-side comparison

| Aspect | Kconfig | Devicetree |
|---|---|---|
| **Origin** | Linux kernel | OpenFirmware / PowerPC booting, adopted by Linux, then Zephyr |
| **File syntax** | Kconfig language (`config FOO`, `bool "..."`, `depends on`) | Devicetree source (`.dts` / `.dtsi`) — node/property tree |
| **Schema** | `Kconfig` files (define symbols) | **Bindings** (`*.yaml` in `zephyr/dts/bindings/`) validate properties |
| **Inputs** | Zephyr Kconfig files, board `defconfig`, `prj.conf`, extra `.conf` overlays | board `.dts` + included `.dtsi`, SoC files, `app.overlay`, `*.overlay` |
| **Resolved into** | `build/.config` → `build/zephyr/include/generated/autoconf.h` | `build/zephyr/include/generated/devicetree_generated.h` (+ `zephyr.dts`) |
| **In C code** | `#if defined(CONFIG_GPIO)` | `GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios)` |
| **Value types** | bool, int, hex, string, choice | integers, strings, arrays, phandles (`&gpioc`), cell tuples (`<...>`) |
| **Granularity** | Feature/subsystem level ("enable UART console") | Instance level ("UART1 on PA9/PA10", "this sensor on I2C1 @ 0x40") |
| **Who writes it** | App developer enables options; maintainers define them | Board/SoC maintainers write it; app developers *overlay* it |
| **Runtime changeable?** | No (rebuild required) | No (rebuild required) |
| **Enforces** | Dependencies (`depends on`), defaults, auto-selection (`select`) | Bindings (property names/types), node status (`status = "okay"/"disabled"`) |

---

## 4. Kconfig in depth

### 4.1 What it looks like

A **definition** (usually written by subsystem maintainers, e.g. in `zephyr/drivers/gpio/Kconfig`):

```kconfig
config GPIO
    bool "GPIO drivers"
    depends on HAS_GPIO
    help
      Enable GPIO driver support.
```

An **assignment** (written by you, in `prj.conf`):

```conf
CONFIG_GPIO=y
```

### 4.2 Where values come from — priority hierarchy

```
overlay .conf files   ← highest priority (west build -- -DOVERLAY_CONFIG=extra.conf)
prj.conf              ← your application
board defconfig       ← what the board needs to boot (zephyr/boards/weact/blackpill_f401cc/blackpill_f401cc_defconfig)
Zephyr Kconfig files  ← subsystem defaults (the `default` lines)
```

### 4.3 How it reaches C code

```
Kconfig files ──► kconfiglib ──► build/.config ──► autoconf.h
                                                       │
                                    #define CONFIG_GPIO 1  ◄── #if defined(CONFIG_GPIO)
```

### 4.4 Typical things Kconfig controls

- `CONFIG_GPIO=y` — compile the GPIO subsystem and its drivers
- `CONFIG_LOG=y`, `CONFIG_LOG_DEFAULT_LEVEL=3` — logging
- `CONFIG_SHELL=y` — interactive shell
- `CONFIG_BT=y` / `CONFIG_NETWORKING=y` — big feature stacks
- `CONFIG_SERIAL=y`, `CONFIG_CONSOLE=y` — console plumbing
- `CONFIG_BOOT_BANNER=y` — the "*** Booting Zephyr OS ***" line
- Your own app options: `config APP_ENABLE_BACKGROUND_WORK ...` (see the Kconfig README)

---

## 5. Devicetree in depth

### 5.1 What it looks like

Board file (`zephyr/boards/weact/blackpill_f401cc/blackpill_f401cc.dts`, excerpts):

```dts
/ {
    aliases {
        led0 = &user_led;          /* the name your app uses */
        sw0 = &user_button;
        zephyr,console = &usart1;  /* where printk/log output goes */
    };

    user_led: user-led {
        compatible = "gpio-leds";
        gpios = <&gpioc 13 GPIO_ACTIVE_LOW>;   /* PC13, active-low */
    };

    user_button: user-button {
        compatible = "gpio-keys";
        gpios = <&gpioa 0 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
    };
};

&usart1 {
    pinctrl-0 = <&usart1_tx_pa9 &usart1_rx_pa10>;
    current-speed = <115200>;
    status = "okay";
};
```

Key concepts:

| Concept | Meaning |
|---|---|
| **Node** | A hardware thing: LED, UART, sensor, flash... |
| **Property** | A fact about the node: `gpios`, `current-speed`, `reg`, `status` |
| **`compatible`** | The driver this node belongs to (`"gpio-leds"` → the generic LED driver) |
| **`reg`** | MMIO address / I2C address / SPI chip-select — "where on the bus" |
| **`gpios = <&gpioc 13 GPIO_ACTIVE_LOW>`** | A **phandle** (`&gpioc`) + pin number + flags — exactly what `struct gpio_dt_spec` needs |
| **`status = "okay"` / `"disabled"`** | Whether the node is active — disabled nodes generate no driver instance |
| **`.dtsi`** | Include files: SoC dtsi (from `zephyr/dts/arm/`) defines all peripherals; board dts enables and pins them |
| **Binding (`*.yaml`)** | The schema that validates each `compatible`'s properties |

### 5.2 Overlays — how *you* change hardware description without touching board files

`app.overlay` (lives in your project root, picked up automatically by the build):

```dts
/ {
    aliases {
        led0 = &my_led;            /* redirect led0 to my pin */
    };

    my_led: my-led {
        compatible = "gpio-leds";
        gpios = <&gpioc 14 GPIO_ACTIVE_LOW>;   /* move LED to PC14 */
    };
};
```

Same mechanism as Kconfig overlays (`extra.conf`) — your changes sit on top of the
board's description and win.

### 5.3 How it reaches C code

```
.dts + .dtsi + app.overlay ──► dtc/gen_defines.py ──► devicetree_generated.h
                                                             │
            DT_ALIAS(led0) ──► node id ──► GPIO_DT_SPEC_GET ──► { .port, .pin=13, .dt_flags=ACTIVE_LOW }
```

Unlike Kconfig (`#if`), Devicetree values are **always compiled in as constants** —
there's no "disabled" runtime cost question; unused nodes are simply not referenced.

---

## 6. How they interact in one build

The two systems merge inside a single Zephyr build, each feeding the drivers:

```
                        ┌────────────────────────────┐
   Kconfig  ──►  "compile the GPIO driver subsystem" │
                        │                            │
   Devicetree ─►  "the LED is &gpioc pin 13,         │
                        │   active-low, named led0"   │
                        └───────────┬────────────────┘
                                    ▼
              drivers/gpio/gpio_stm32.c  (compiled because CONFIG_GPIO=y,
                                          bound to PC13 because of the dts)
                                    ▼
                         gpio_pin_toggle_dt(&led)
                                    ▼
                               PC13 toggles
```

Two consequences worth remembering:

1. **A node without its Kconfig driver = nothing.** If `prj.conf` lacks
   `CONFIG_GPIO=y`, the LED node exists in Devicetree but no GPIO driver code is
   compiled — the app won't build/work.
2. **A Kconfig driver without a node = nothing.** If the board dts never enables
   `&usart1` (`status = "okay"`), enabling `CONFIG_SERIAL=y` gives you no console —
   there's no device instance for the driver to bind to.

> They are complementary: **Kconfig = "compile it", Devicetree = "wire it".**

---

## 7. Black Pill example — same project, both systems

Project: LED toggle on `blackpill_f401cc` (see the project README). Watch both
systems at work:

### From Devicetree (board file, already provided)

```dts
aliases { led0 = &user_led; };
user_led: user-led {
    compatible = "gpio-leds";
    gpios = <&gpioc 13 GPIO_ACTIVE_LOW>;
};
```

### From Kconfig (your `prj.conf`)

```conf
CONFIG_GPIO=y          # ← without this, no GPIO driver is compiled at all
```

### In your C code — both meet

```c
#include <zephyr/drivers/gpio.h>

/* Devicetree provides the pin/device facts: */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
/* expands to: { .port = <gpioc device>, .pin = 13, .dt_flags = GPIO_ACTIVE_LOW } */

int main(void)
{
    /* Kconfig decided this API/driver exists in the binary: */
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    while (1) {
        gpio_pin_toggle_dt(&led);   /* respects ACTIVE_LOW from Devicetree */
        k_msleep(500);
    }
}
```

### Exercise: change the pin (Devicetree job)

Add `app.overlay`: `gpios = <&gpioc 14 GPIO_ACTIVE_LOW>;` → rebuild → LED moves to PC14.
`main.c` untouched. **No Kconfig involved.**

### Exercise: add a second feature (Kconfig job)

Add to `prj.conf`: `CONFIG_LOG=y`, `CONFIG_SERIAL=y`, `CONFIG_CONSOLE=y` → rebuild →
you get log output on USART1 (PA9/PA10 — *that* wiring is Devicetree's `zephyr,console`
alias + pinctrl). **No DTS edit involved.**

---

## 8. How to know which one you need

Ask yourself:

| Your need | System | Where you edit |
|---|---|---|
| Turn a feature/driver on or off | Kconfig | `prj.conf` |
| Set a software parameter (log level, buffer size, shell history) | Kconfig | `prj.conf` |
| Add an app-specific on/off option | Kconfig | your app's `Kconfig` file + `prj.conf` |
| Change which pin an LED/button/sensor uses | Devicetree | `app.overlay` |
| Enable an on-chip peripheral (USART1, I2C1, SPI1, ADC) | Devicetree | `app.overlay` (`status = "okay"` + pinctrl) |
| Attach an external sensor (I2C address, SPI CS) | Devicetree | `app.overlay` |
| Change baud rate / buffer sizes of a peripheral | Devicetree | `app.overlay` (`current-speed`, etc.) |
| Redirect console to another UART | Devicetree | `app.overlay` (`zephyr,console = &usartX`) |
| The build says "undefined reference to <driver API>" | Kconfig missing | enable it in `prj.conf` |
| The build says "node not found / no such alias" | Devicetree missing | fix alias/node in `.dts`/`.overlay` |

---

## 9. Common confusions

1. **"I enabled `CONFIG_GPIO=y` — why doesn't the LED work?"**
   Devicetree side missing: no `led0` alias/node, or the node is disabled. Check the
   board dts and your overlay.

2. **"The dts defines USART1 perfectly — why no console output?"**
   Kconfig side missing: `CONFIG_SERIAL=y` / `CONFIG_CONSOLE=y` not enabled.

3. **"Can I use Kconfig to pick the LED pin?"**
   Technically you could write `config LED_PIN int ...`, but that is the *wrong tool*:
   pin/polarity/device facts belong to Devicetree, where drivers can consume them via
   `gpio_dt_spec`, pinctrl, and bindings. Use `app.overlay`.

4. **"Can I use Devicetree to enable Bluetooth?"**
   Devicetree describes the BT *hardware* (radio node, antennas, HCI transport), but
   the *protocol stack* is software — enabled via Kconfig (`CONFIG_BT=y`). Big
   features need both, each for its own half.

5. **"Are `.conf` overlays and `.dts` overlays the same thing?"**
   No. `-DOVERLAY_CONFIG=extra.conf` is Kconfig; `app.overlay` / `file.overlay` is
   Devicetree. Different syntax, different merge step.

6. **"Where do I put my changes?"**
   Board files (`.dts`, `_defconfig`) are maintained upstream — never edit them.
   App changes go in `prj.conf` (Kconfig) and `app.overlay` (Devicetree).

---

## 10. File reference

| File | System | Role |
|---|---|---|
| `zephyr/Kconfig`, subsystem `Kconfig` files | Kconfig | symbol definitions |
| `zephyr/boards/<v>/<b>/<b>_defconfig` | Kconfig | board defaults |
| `prj.conf` | Kconfig | your app's settings |
| `extra.conf` via `OVERLAY_CONFIG` | Kconfig | optional override layer |
| `build/.config`, `build/zephyr/include/generated/autoconf.h` | Kconfig | resolved output |
| `zephyr/dts/arm/.../*.dtsi` | Devicetree | SoC peripheral descriptions |
| `zephyr/dts/bindings/**/*.yaml` | Devicetree | property schemas per `compatible` |
| `zephyr/boards/<v>/<b>/<b>.dts` | Devicetree | board wiring + aliases |
| `app.overlay` / `*.overlay` | Devicetree | your overrides |
| `build/zephyr/zephyr.dts`, `build/zephyr/include/generated/devicetree_generated.h` | Devicetree | resolved output |

---

## 11. Cheat sheet

```
KCONFIG  = software:  WHAT to compile     → prj.conf        → autoconf.h      → #if defined(CONFIG_X)
DEVICETREE = hardware: WHERE it is wired   → app.overlay     → devicetree_generated.h → DT_ALIAS(...)/GPIO_DT_SPEC_GET

Dependency rule:  node (DTS) + driver (Kconfig)  →  working device
Wrong-pin fix?        → Devicetree overlay
Missing-feature fix?  → prj.conf
```

```bash
west build -t menuconfig     # inspect/adjust Kconfig for current build
cat build/zephyr/zephyr.dts  # inspect the fully merged Devicetree
```

---

*Related docs: "Zephyr Kconfig — Complete Guide" and "Zephyr Project from Scratch —
Black Pill LED Toggle" in this series.*
