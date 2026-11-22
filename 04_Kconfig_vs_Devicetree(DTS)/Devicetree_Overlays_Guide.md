# Devicetree Overlays — Where to Find Properties and How to Write Them

> Companion guide to [Create a New Zephyr Project](../03_Create_New_Project/README.md),
> [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md),
> and this folder's [Kconfig vs Devicetree guide](README.md).
>
> Use this when you need to **change hardware**: move an LED to another pin,
> enable a UART, change an RGB color order, add an external LED, or disable a
> peripheral.

---

## 1. Three names you must not confuse

| Item | Purpose | Example |
|---|---|---|
| **Kconfig symbol** | Enables software support | `CONFIG_UART_ESP32` |
| **`compatible` string** | Identifies the hardware type and its binding | `espressif,esp32-uart` |
| **Node label** | Lets you reference a particular hardware instance | `uart1`, `gpio0`, `led0` |

A **node label is not the driver name**. Devicetree matches nodes to bindings
using their `compatible` properties.

---

## 2. Read the existing hardware description first

Start with the board file:

```text
zephyr/boards/weact/esp32s3_mini/weact_esp32s3_mini_procpu.dts
```

Look for:

- included `.dtsi` files
- existing GPIO, UART, and LED nodes
- references beginning with `&`
- `compatible` and `status` properties

Some controller definitions live in included SoC files, not in the board file.
After a build, **`build/zephyr/zephyr.dts` shows the combined hardware
description**, which is easier to inspect:

```bash
grep -n "gpio0\|gpio1\|uart0\|rgb0\|led0" build/zephyr/zephyr.dts
```

> Do not assume a board directory matches your physical board exactly. Check the
> board target with `west boards | grep -i <name>`.

---

## 3. Find the binding — the “manual” for the node

Bindings are YAML files under:

```text
zephyr/dts/bindings/
```

They tell you:

- the exact `compatible` string
- required and optional properties
- property types and allowed values
- child-node requirements
- included bindings that add more properties

### Example: `gpio-leds`

```text
zephyr/dts/bindings/led/gpio-leds.yaml
```

Full contents:

```yaml
# Copyright (c) 2018, Linaro Limited
# SPDX-License-Identifier: Apache-2.0

description: |
  Group of GPIO-controlled LEDs.

  Each LED is defined in a child node of the gpio-leds node.

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
        /*
         * led_0 is pin 1 on gpio0.
         * The LED is on when the pin is low, and off when the pin is high.
         */
        led_0 {
          gpios = <&gpio0 1 GPIO_ACTIVE_LOW>;
        };
        /*
         * led_1 is pin 2 on gpio0.
         * The LED is on when the pin is high, and off when it is low.
         */
        led_1 {
          gpios = <&gpio0 2 GPIO_ACTIVE_HIGH>;
        };
        /*
         * led_2 is pin 15 on gpio1.
         * The LED is on when the pin is low, and the pin's internal pull-up resistor should be
         * enabled.
         */
        led_2 {
          gpios = <&gpio1 15 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
        };
      };
    };
```

### What the binding tells us

- Parent node `compatible = "gpio-leds"`.
- Each LED is a **child node**.
- Each child **must** have `gpios` (type `phandle-array`).
- The child may include properties from `led-node.yaml`, but only `label` is
  allowed here.
- The `gpios` value has three parts:

```dts
gpios = <&controller pin flags>;
```

### Included binding: `led-node.yaml`

```yaml
# Copyright (c) 2025 Seagate Technology LLC
# SPDX-License-Identifier: Apache-2.0

# Common fields for LED nodes

properties:
  label:
    type: string
    description: |
      Human readable string describing the LED. It can be used by an
      application to identify this LED or to retrieve its number/index
      (i.e. child node number) on the parent device.
  color-mapping:
    type: array
    description: |
      Channel to color mapping of a multicolor LED. If a LED supports
      several colors, then the color-mapping property can be used to
      describe how the hardware channels and the colors are mapped.

      For example the channel to color mapping of RGB LEDs would be

         color-mapping =
              <LED_COLOR_ID_RED>,
              <LED_COLOR_ID_GREEN>,
              <LED_COLOR_ID_BLUE>;
```

For a **single-color external LED**, you only need:

- `gpios` (required)
- `label` (optional, human-readable)

You do **not** need `color-mapping`.

---

## 4. Work out a GPIO7 LED overlay property by property

Assume an external LED is connected to **GPIO7**.

### 4.1 Find the controller

GPIO7 is on the ESP32-S3 `gpio0` controller (pins 0–31):

```bash
grep -n "gpio0:" zephyr/dts/xtensa/espressif/esp32s3/esp32s3_common.dtsi
```

Result:

```dts
gpio0: gpio@60004000 {
    compatible = "espressif,esp32-gpio";
    gpio-controller;
    #gpio-cells = <2>;
    ngpios = <32>;  /* 0..31 */
};
```

So the controller reference is:

```dts
&gpio0
```

The board already enables it:

```dts
&gpio0 {
    status = "okay";
};
```

### 4.2 Find the pin number

The pin number is the physical GPIO number: **7**.

```dts
&gpio0 7
```

### 4.3 Choose the flags

Flags come from:

```text
zephyr/include/zephyr/dt-bindings/gpio/gpio.h
```

Important macros:

```c
#define GPIO_ACTIVE_LOW   (1 << 0)
#define GPIO_ACTIVE_HIGH  (0 << 0)
#define GPIO_PULL_UP      (1 << 4)
#define GPIO_PULL_DOWN    (1 << 5)
```

Choose based on how the LED is wired:

| Wiring | Flag |
|---|---|
| GPIO → resistor → LED anode → GND | `GPIO_ACTIVE_HIGH` |
| 3V3 → resistor → LED anode → GPIO (sink) | `GPIO_ACTIVE_LOW` |
| No external pull resistor and you want an internal pull-up | add `GPIO_PULL_UP` |
| No external pull resistor and you want an internal pull-down | add `GPIO_PULL_DOWN` |

Combine flags with `|`, for example:

```dts
GPIO_PULL_UP | GPIO_ACTIVE_LOW
```

For a normal active-high LED:

```dts
GPIO_ACTIVE_HIGH
```

### 4.4 Choose the node label and alias

- Node label: `external_led` (used by `DT_NODELABEL(external_led)`).
- Alias: `led0` (used by `DT_ALIAS(led0)`, which is what the standard blinky
  sample expects).

The alias is optional, but it makes the project compatible with existing code.

### 4.5 Write the overlay skeleton

```dts
#include <zephyr/dt-bindings/gpio/gpio.h>

/ {
    leds {
        compatible = "gpio-leds";

        external_led: led_7 {
            gpios = <&gpio0 7 GPIO_ACTIVE_HIGH>;
            label = "External LED on GPIO7";
        };
    };

    aliases {
        led0 = &external_led;
    };
};
```

### 4.6 What each line means

| Line | Meaning |
|---|---|
| `#include <zephyr/dt-bindings/gpio/gpio.h>` | Makes `GPIO_ACTIVE_HIGH`, `GPIO_ACTIVE_LOW`, etc. available. |
| `/ {` | Adds nodes under the root of the Devicetree. |
| `leds {` | Parent node name. The name is not important; `leds` is conventional. |
| `compatible = "gpio-leds";` | Tells Zephyr this is a group of GPIO LEDs. Matches the binding. |
| `external_led: led_7 {` | Child node with label `external_led` and node name `led_7`. |
| `gpios = <&gpio0 7 GPIO_ACTIVE_HIGH>;` | Controller `gpio0`, pin `7`, active-high. |
| `label = "External LED on GPIO7";` | Optional human-readable description. |
| `aliases { led0 = &external_led; }` | Makes `DT_ALIAS(led0)` point to the new LED. |

### 4.7 If the LED is active-low

Change only the flag:

```dts
gpios = <&gpio0 7 GPIO_ACTIVE_LOW>;
```

The C code does not change. `gpio_pin_toggle_dt()` respects the flag.

---

## 5. Where to get each overlay value

| Value | Where to find it |
|---|---|
| Controller label (`&gpio0`) | Board `.dts`, SoC `.dtsi`, `build/zephyr/zephyr.dts` |
| Pin number | Board schematic, pinout diagram, board documentation |
| Flags (`GPIO_ACTIVE_*`, `GPIO_PULL_*`) | `zephyr/include/zephyr/dt-bindings/gpio/gpio.h` |
| `compatible` string | Binding YAML (`compatible:`) |
| Property names (`gpios`, `label`) | Binding YAML (`properties:`) |
| Required properties | Binding YAML (`required: true`) |
| Allowed values / types | Binding YAML (`type:`, `enum:`, examples) |
| Child-node rules | Binding YAML (`child-binding:`) |
| Existing examples | `zephyr/samples/**/boards/*.overlay` |

Useful commands:

```bash
# Find the controller in the SoC/board dtsi
grep -n "gpio0:" zephyr/dts/xtensa/espressif/esp32s3/esp32s3_common.dtsi

# See the final merged devicetree
grep -n "gpio0\|gpio1\|leds\|led0" build/zephyr/zephyr.dts

# Find flag macros
grep -n "GPIO_ACTIVE_LOW\|GPIO_PULL_UP" \
  zephyr/include/zephyr/dt-bindings/gpio/gpio.h

# Find bindings and examples
find zephyr/dts/bindings -iname "*gpio-leds*"
find zephyr/samples -path "*/boards/*.overlay" | head
```

---

## 6. Overlay file placement and selection

The build system automatically looks for overlays in the application folder.

| File | Applies to |
|---|---|
| `app.overlay` | All boards |
| `boards/<board>.overlay` | One board |
| `socs/<soc>_<qualifiers>.overlay` | One SoC + board qualifier combination |
| `boards/<board>_<revision>.overlay` | One board revision |

For multi-core boards, the per-board overlay name includes the core. For example,
the ESP32-S3 Devkit uses:

```text
boards/esp32s3_devkitc_procpu.overlay
```

For the WeAct ESP32-S3 Mini, use:

```text
boards/weact_esp32s3_mini_procpu.overlay
```

or simply use `app.overlay`.

### Precedence

Overlays are concatenated **after** the board `.dts`, so they can override it.
If several overlays are used, later ones override earlier ones.

### Explicit selection

```bash
west build -b <board> -d build_app . -- \
  -DDTC_OVERLAY_FILE=my_board.overlay
```

or add an extra overlay:

```bash
west build -b <board> -d build_app . -- \
  -DEXTRA_DTC_OVERLAY_FILE=extra.overlay
```

Check the build output for:

```text
Found devicetree overlay: .../app.overlay
```

---

## 7. Common overlay operations

### 7.1 Add a new node (external LED)

```dts
/ {
    leds {
        compatible = "gpio-leds";

        external_led: led_7 {
            gpios = <&gpio0 7 GPIO_ACTIVE_HIGH>;
            label = "External LED on GPIO7";
        };
    };

    aliases {
        led0 = &external_led;
    };
};
```

### 7.2 Modify an existing node

```dts
&uart1 {
    status = "okay";
    current-speed = <115200>;
};
```

### 7.3 Disable an existing node

```dts
&uart1 {
    status = "disabled";
};
```

### 7.4 Change an alias

```dts
/ {
    aliases {
        led0 = &my_led;
    };
};
```

### 7.5 Change the RGB color order

```dts
#include <zephyr/dt-bindings/led/led.h>

&rgb_led {
    color-mapping = <LED_COLOR_ID_RED
                     LED_COLOR_ID_GREEN
                     LED_COLOR_ID_BLUE>;
};
```

### 7.6 Enable UART1 on the ESP32-S3 Mini

The board pinctrl already defines `uart1_default`, so:

```dts
&uart1 {
    status = "okay";
    current-speed = <115200>;
    pinctrl-0 = <&uart1_default>;
    pinctrl-names = "default";
};
```

Software side:

```conf
CONFIG_SERIAL=y
```

`CONFIG_UART_ESP32` is enabled automatically because the `uart1` node exists.

Code:

```c
#include <zephyr/drivers/uart.h>
const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart1));
```

### 7.7 Move the Black Pill LED to PC14

```dts
#include <zephyr/dt-bindings/gpio/gpio.h>

/ {
    leds {
        compatible = "gpio-leds";

        my_led: led_1 {
            gpios = <&gpioc 14 GPIO_ACTIVE_LOW>;
            label = "My LED";
        };
    };

    aliases {
        led0 = &my_led;
    };
};
```

---

## 8. Enable both sides: hardware and software

There are usually **two sides** to check:

### Devicetree side

- The node must be enabled: `status = "okay";`
- Parent bus must be enabled.
- Pin routing / pinctrl must be correct.
- Required properties must be present.

### Kconfig side

- The subsystem and driver must be enabled in the final `.config`.

> `status = "okay"` alone does **not** guarantee a driver is compiled or that the
> device initializes. Enabling a driver in `prj.conf` does **not** fix a disabled
> or wrongly configured hardware node.

For a plain external LED controlled with the GPIO API:

```conf
CONFIG_GPIO=y
```

If you want to use the LED API instead (`led_on()`, `led_off()`):

```conf
CONFIG_LED=y
```

`CONFIG_LED_GPIO` is enabled by default when `CONFIG_LED=y` and a `gpio-leds`
node exists.

---

## 9. Verify the overlay

After building:

```bash
# Confirm the overlay was loaded
# (look for "Found devicetree overlay" in the build output)

# Inspect the final hardware description
grep -n "external_led\|led0\|gpios" build/zephyr/zephyr.dts

# Inspect generated macros used by C code
grep -n "external_led\|led0" \
  build/zephyr/include/generated/zephyr/devicetree_generated.h

# Inspect the final software configuration
grep CONFIG_GPIO build/zephyr/.config
```

If results look stale:

```bash
west build -p always -b <board> -d build_app <app>
```

---

## 10. Rules and pitfalls

1. **Do not edit the board files.** Put your changes in an overlay.
2. **Use node labels** (`&uart1`, `&gpio0`) instead of long paths when possible.
3. **Include binding headers** when you use macros such as `GPIO_ACTIVE_LOW` or
   `LED_COLOR_ID_RED`.
4. **A node label is not a driver.** The `compatible` string selects the binding
   and driver.
5. **A missing `status` usually means “okay”**, but if the SoC/board file sets
   `status = "disabled"`, you must set `status = "okay"` in your overlay.
6. **`status = "okay"` is not enough.** The matching Kconfig subsystem/driver
   must also be enabled.
7. **Check the binding for required properties.** The build fails if a required
   property is missing.
8. **Overlays override the board.** If you redefine the same property, the
   overlay value wins.
9. **Avoid spaces in project paths.** Zephyr/CMake can fail on paths containing
   spaces.

---

## 11. Cheat sheet

```bash
# Find the board
west boards | grep -i <name>

# Inspect the board devicetree
grep -n "aliases\|compatible\|status\|gpios" \
  zephyr/boards/<vendor>/<board>/<board>.dts

# Inspect the final merged devicetree
grep -n "led0\|rgb0\|gpio0\|uart1" build/zephyr/zephyr.dts

# Find the binding
find zephyr/dts/bindings -iname "*gpio-leds*"
find zephyr/dts/bindings -iname "*ws2812*"

# Read the binding
cat zephyr/dts/bindings/led/gpio-leds.yaml

# Find GPIO flag macros
grep -n "GPIO_ACTIVE_LOW\|GPIO_PULL_UP" \
  zephyr/include/zephyr/dt-bindings/gpio/gpio.h

# Build with an overlay
west build -p always -b <board> -d build_app <app>

# Verify
grep -n "led0\|external_led" build_app/zephyr/zephyr.dts
grep CONFIG_GPIO build_app/zephyr/.config
```

---

## Related documents

- [Create a New Zephyr Project](../03_Create_New_Project/README.md)
- [Kconfig vs Devicetree](README.md)
- [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md)
- [Using Drivers and Software Modules](../03_Create_New_Project/Using_Drivers_And_Software_Modules.md)
- [Kconfig deep dive](../05_KConfig/README.md)
