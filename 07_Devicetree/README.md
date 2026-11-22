# Zephyr Devicetree — Complete Guide to Hardware Description

> **Scope:** This is the **Devicetree deep dive**: how Zephyr describes hardware
> at build time. For the side-by-side comparison with Kconfig, see
> [Kconfig vs Devicetree](<../04_Kconfig_vs_Devicetree(DTS)/README.md>).
>
> **Learning path:** `03 Create Project` → `04 Kconfig vs Devicetree` →
> `05 Kconfig deep dive` → **`07 Devicetree`** → `06 Custom Module` /
> `08 Create Custom Devicetree`.
>
> **Companion guides:**
> - [Devicetree Overlays Guide](<Devicetree_Overlays_Guide.md>)
>   — where to find binding properties and how to write overlays.
> - [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md)
>   — how to find the right `CONFIG_...` option.
> - [Using Drivers and Software Modules](../03_Create_New_Project/Using_Drivers_And_Software_Modules.md)
>   — the general lifecycle for using any driver, subsystem, or module.

---

## Table of Contents

1. [What is Devicetree?](#1-what-is-devicetree)
2. [Why Zephyr needs it](#2-why-zephyr-needs-it)
3. [Build-time vs run-time](#3-build-time-vs-run-time)
4. [Core concepts](#4-core-concepts)
5. [Syntax](#5-syntax)
6. [How it reaches C code](#6-how-it-reaches-c-code)
7. [Bindings — the schema for a node](#7-bindings--the-schema-for-a-node)
8. [Overlays — how you change hardware description](#8-overlays--how-you-change-hardware-description)
9. [Kconfig and Devicetree together](#9-kconfig-and-devicetree-together)
10. [Inspecting the generated files](#10-inspecting-the-generated-files)
11. [Troubleshooting](#11-troubleshooting)
12. [Cheat sheet](#12-cheat-sheet)
13. [Key takeaways](#13-key-takeaways)

---

## 1. What is Devicetree?

**Devicetree is a data structure that describes hardware.** It originated in
OpenFirmware / PowerPC booting, was adopted by Linux, and is now used by
Zephyr to describe every board, SoC, and peripheral.

Instead of hard-coding pins, addresses, and bus speeds in C, you describe the
hardware once in a `.dts` file. The build system turns that description into
C macros and constants that your code and the drivers consume.

---

## 2. Why Zephyr needs it

Embedded hardware varies enormously:

- Different boards wire the same SoC pin to different LEDs, buttons, sensors.
- The same driver must work on many boards without recompilation of its source.
- Hardware is a fact of the board — it does not change at run time.

Devicetree solves this by separating **hardware facts** from **software logic**:

- Board maintainers describe the hardware once (`.dts`).
- Driver authors write generic code that reads the description.
- Application authors reference hardware by name (`led0`, `rgb0`, `uart0`).

---

## 3. Build-time vs run-time

| Aspect | Devicetree | Run-time config (e.g. `if` in code) |
|---|---|---|
| Decision moment | Build time | While the chip runs |
| Disabled node | No driver instance generated | Still in flash, just skipped |
| Cost when unused | Zero (unused nodes are not referenced) | Flash/RAM still occupied |
| How it is expressed | `.dts` / `.dtsi` / `.overlay` | Ordinary C statements |
| Can change after flashing? | No (rebuild required) | Yes |

Devicetree values are **always compiled in as constants**. There is no
"disabled at run time" question.

---

## 4. Core concepts

| Concept | Meaning |
|---|---|
| **Node** | A hardware thing: LED, UART, sensor, flash, GPIO controller... |
| **Property** | A fact about the node: `gpios`, `current-speed`, `reg`, `status` |
| **`compatible`** | The binding/driver this node belongs to (`"gpio-leds"`, `"worldsemi,ws2812-i2s"`) |
| **`reg`** | MMIO address / I2C address / SPI chip-select — "where on the bus" |
| **Phandle** | A reference to another node, written `&node_name` (e.g. `&gpioc`) |
| **Cell tuple** | `<&controller pin flags>` — a group of integer cells |
| **Alias** | A short name for a node (`led0`, `rgb0`, `sw0`) used by applications |
| **`chosen`** | Special node that selects system defaults (`zephyr,console`, `zephyr,shell-uart`) |
| **`status`** | `"okay"` (active) or `"disabled"` (inactive) — disabled nodes generate no driver |
| **`.dtsi`** | Include file: SoC `.dtsi` defines all peripherals; board `.dts` enables and pins them |
| **Binding (`*.yaml`)** | The schema that validates each `compatible`'s properties |
| **Overlay** | A file that sits on top of the board description and overrides it |

---

## 5. Syntax

### 5.1 A minimal board excerpt

```dts
/dts-v1/;

#include <st/f4/stm32f401Xc.dtsi>
#include <zephyr/dt-bindings/gpio/gpio.h>

/ {
    model = "WeAct Studio Black Pill";
    compatible = "weact,blackpill-f401ce";

    aliases {
        led0 = &user_led;          /* name your app uses */
        sw0  = &user_button;
        zephyr,console = &usart1;  /* where printk output goes */
    };

    leds {
        compatible = "gpio-leds";  /* parent binding */

        user_led: led {
            gpios = <&gpioc 13 GPIO_ACTIVE_LOW>;   /* PC13, active-low */
        };
    };

    gpio_keys {
        compatible = "gpio-keys";

        user_button: button {
            gpios = <&gpioa 0 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
        };
    };
};

&usart1 {
    pinctrl-0 = <&usart1_tx_pa9 &usart1_rx_pa10>;
    current-speed = <115200>;
    status = "okay";
};
```

### 5.2 Key syntax rules

- `/dts-v1/;` declares the Devicetree version (always present).
- `/ { ... };` is the root node.
- `&node { ... };` modifies an existing node (reference by label).
- `label: name { ... };` creates a node with a label you can reference with `&label`.
- `#include <...>` includes other files (SoC `.dtsi`, binding headers, pinctrl).
- Properties are `name = <value>;` — values can be integers, strings, arrays, phandles.
- `compatible = "vendor,device";` is the most important property — it selects the binding.
- Children of a parent node are indented inside `{ }`.
- Comments use `/* ... */` (C style) or `//` (line style in newer dtc).

### 5.3 Property value types

| Type | Example | Meaning |
|---|---|---|
| Integer | `reg = <0x40021000 0x400>;` | Address + size |
| String | `model = "WeAct Black Pill";` | Free text |
| String list | `compatible = "st,stm32f401", "st,stm32f4";` | Multiple compatibles |
| Phandle | `interrupt-parent = <&intc>;` | Reference to another node |
| Cell tuple | `gpios = <&gpioc 13 GPIO_ACTIVE_LOW>;` | Controller + pin + flags |
| Boolean | `status = "okay";` | Present/absent (often string form) |

---

## 6. How it reaches C code

The build pipeline:

```
.dts + .dtsi + app.overlay
        │
        ▼
  dtc / gen_defines.py
        │
        ▼
  build/zephyr/zephyr.dts
  build/zephyr/include/generated/zephyr/devicetree_generated.h
        │
        ▼
  C macros: DT_ALIAS(led0), DT_NODELABEL(uart1), GPIO_DT_SPEC_GET(...)
```

Common macros you use in application code:

| Macro | Use |
|---|---|
| `DT_ALIAS(name)` | Get the node id for an alias |
| `DT_NODELABEL(label)` | Get the node id for a label |
| `DT_NODE_EXISTS(node)` | Check whether a node exists |
| `DT_NODE_HAS_STATUS(node, okay)` | Check node status |
| `GPIO_DT_SPEC_GET(node, gpios)` | Extract controller + pin + flags |
| `DEVICE_DT_GET(node)` | Get the `struct device *` for a node |
| `DT_PROP(node, property)` | Read a property value |
| `DT_FOREACH_CHILD(node, fn)` | Iterate child nodes |

Example:

```c
#include <zephyr/drivers/gpio.h>

#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

int main(void)
{
    if (!gpio_is_ready_dt(&led)) {
        return -1;
    }
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    while (1) {
        gpio_pin_toggle_dt(&led);
        k_msleep(500);
    }
}
```

---

## 7. Bindings — the schema for a node

A binding is a YAML file under `zephyr/dts/bindings/`. It validates the
properties of a node based on its `compatible` string.

Example: `zephyr/dts/bindings/led/gpio-leds.yaml`

```yaml
compatible: "gpio-leds"

child-binding:
  description: GPIO LED child node
  include:
    - name: led-node.yaml
      property-allowlist:
        - label
  properties:
    gpios:
      type: phandle-array
      required: true

examples:
  - |
    / {
      leds {
        compatible = "gpio-leds";
        led_0 {
          gpios = <&gpio0 1 GPIO_ACTIVE_LOW>;
        };
      };
    };
```

What the binding tells you:

- The parent node must have `compatible = "gpio-leds"`.
- Each LED is a child node.
- Each child **must** have `gpios` (type `phandle-array`).
- The child may include `label` (from `led-node.yaml`).
- The `examples` block shows valid usage.

Bindings are the **manual** for a node. Always read the binding before using
a new `compatible`.

---

## 8. Overlays — how you change hardware description

You should never edit the board's `.dts` files (they are maintained upstream).
Instead, put your changes in an overlay.

| File | Applies to |
|---|---|
| `app.overlay` | All boards |
| `boards/<board>.overlay` | One board |
| `socs/<soc>_<qualifiers>.overlay` | One SoC + board qualifier combination |
| `boards/<board>_<revision>.overlay` | One board revision |

For multi-core boards, the overlay name includes the core:
`boards/esp32s3_devkitc_procpu.overlay`.

Overlays are concatenated **after** the board `.dts`, so they override it.

Explicit selection:

```bash
west build -b <board> -d build_app . -- \
  -DDTC_OVERLAY_FILE=my_board.overlay
# or
west build -b <board> -d build_app . -- \
  -DEXTRA_DTC_OVERLAY_FILE=extra.overlay
```

---

## 9. Kconfig and Devicetree together

They are complementary:

- **Kconfig = "compile it"** — `CONFIG_GPIO=y` compiles the GPIO driver.
- **Devicetree = "wire it"** — the `led0` node says which pin.

Two consequences:

1. **A node without its Kconfig driver = nothing.** If `prj.conf` lacks
   `CONFIG_GPIO=y`, the LED node exists in Devicetree but no GPIO driver code
   is compiled.
2. **A Kconfig driver without a node = nothing.** If the board dts never
   enables `&usart1` (`status = "okay"`), enabling `CONFIG_SERIAL=y` gives
   you no console — there is no device instance to bind to.

---

## 10. Inspecting the generated files

After a build:

```bash
# Final merged hardware description
cat build/zephyr/zephyr.dts

# Generated C macros
cat build/zephyr/include/generated/zephyr/devicetree_generated.h

# Final software config
cat build/zephyr/.config
```

Useful greps:

```bash
grep -n "led0\|rgb0\|gpio0\|uart0" build/zephyr/zephyr.dts
grep -n "DT_N_ALIAS_led0\|DT_N_NODELABEL" \
  build/zephyr/include/generated/zephyr/devicetree_generated.h
```

Keep permanent changes in your application files — not these generated outputs.

---

## 11. Troubleshooting

| Symptom | Where to look / what to do |
|---|---|
| "node not found / no such alias" | Fix the alias or node in `.dts`/`.overlay` |
| "undefined reference to <driver API>" | Enable the matching Kconfig symbol in `prj.conf` |
| LED does not blink | Check the node `status`, the GPIO flags, and the polarity |
| Overlay not applied | Check the build log for "Found devicetree overlay" |
| Stale results | Pristine rebuild: `west build -p always` |
| Wrong pin | Override with an overlay; never edit board `.dts` |

---

## 12. Cheat sheet

```bash
# Inspect the board Devicetree
grep -n "aliases\|compatible\|status\|gpios" \
  zephyr/boards/<vendor>/<board>/<board>.dts

# Inspect the final merged Devicetree
grep -n "led0\|rgb0\|gpio0\|uart1" build/zephyr/zephyr.dts

# Find a binding
find zephyr/dts/bindings -iname "*gpio-leds*"
find zephyr/dts/bindings -iname "*ws2812*"

# Read a binding
cat zephyr/dts/bindings/led/gpio-leds.yaml

# Find GPIO flag macros
grep -n "GPIO_ACTIVE_LOW\|GPIO_PULL_UP" \
  zephyr/include/zephyr/dt-bindings/gpio/gpio.h

# Build with an overlay
west build -p always -b <board> -d build_app <app>

# Verify
grep -n "led0\|external_led" build_app/zephyr/zephyr.dts
```

---

## 13. Key takeaways

- **Devicetree = build-time hardware description.** It describes what hardware
  exists and how it is wired.
- **Nodes + properties + `compatible` + bindings** are the four pillars.
- **Aliases** (`led0`, `rgb0`) are the bridge between hardware and application code.
- **Overlays** let you change hardware description without touching board files.
- **Kconfig and Devicetree are complementary**: Kconfig compiles the driver,
  Devicetree wires it.
- **Always read the binding** before using a new `compatible`.
- **Never edit board `.dts` files** — use overlays.

---

## Related documents

- [Create a New Zephyr Project](../03_Create_New_Project/README.md) — full
  project examples.
- [Kconfig vs Devicetree](<../04_Kconfig_vs_Devicetree(DTS)/README.md>) —
  side-by-side comparison.
- [Devicetree Overlays Guide](<Devicetree_Overlays_Guide.md>)
  — where to find overlay properties and how to write overlays.
- [Kconfig deep dive](../05_KConfig/README.md) — the configuration system.
- [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md)
  — how to find the right `CONFIG_...` option.
- [Using Drivers and Software Modules](../03_Create_New_Project/Using_Drivers_And_Software_Modules.md)
  — the general lifecycle for using any driver, subsystem, or module.
- [Create a Custom Devicetree](../08_Create_Custom_Devicetree/README.md) —
  how to define your own binding and hardware description.
- [Custom Module](<../06_Create_Custom_Module_(Digital_To_Volt)/README.md>) —
  how to build an out-of-tree module.
