# Finding and Enabling Features in Zephyr

> Companion guide to [Create a New Zephyr Project](../03_Create_New_Project/README.md)
> and the [Kconfig deep dive](README.md) in this folder.
>
> Use this when you know **what** feature you want (GPIO, UART, RGB LED,
> logging, Bluetooth...) but do not know **which `CONFIG_...` line** to add or
> **which Kconfig file** to read.

---

## Main idea

For each hardware or software feature your application needs:

1. Identify the **Kconfig options**.
2. Check whether it also needs **Devicetree** configuration.
3. Enable the options in `prj.conf`.
4. Use the matching API in C.

You do **not** need to open a Kconfig file for every function you call. Start
with the feature or subsystem — such as GPIO, UART, logging, or Bluetooth.

---

## The three configuration layers

| Layer | Purpose | Where you work |
|---|---|---|
| **Kconfig definitions** | Define available software options, defaults, and dependencies | Zephyr's `Kconfig` files — normally read only |
| **Application configuration** | Select the options your application needs | Your `prj.conf` |
| **Devicetree** | Describe hardware devices, connections, and settings | Board `.dts` files and your application overlay |

**Remember:**

> Kconfig defines the options → `prj.conf` selects them → Devicetree describes
> the hardware → application code uses the APIs.

---

## Four ways to search for a Kconfig option

### 1. `menuconfig` — search interactively

```bash
west build -t menuconfig -d build_<board>
```

Press `/`, type a keyword such as `LED_STRIP`, `UART`, or `GPIO`, then press
Enter. The menu shows:

- the symbol name
- its type (bool, int, hex, string)
- its default value
- its dependencies
- where it is defined

### 2. `guiconfig` — graphical search

```bash
west build -t guiconfig -d build_<board>
```

### 3. Search the source files

```bash
grep -rn "config LED_STRIP" zephyr/drivers/led_strip/Kconfig
grep -rn "config UART"      zephyr/drivers/serial/Kconfig*
grep -rn "config GPIO"      zephyr/drivers/gpio/Kconfig
```

### 4. Use the online Kconfig reference

```text
https://docs.zephyrproject.org/latest/kconfig.html
```

Search for the symbol name without the `CONFIG_` prefix, for example `LED_STRIP`.

---

## Step-by-step workflow

### Step 1 — Identify the feature

Ask what your application needs:

- GPIO output for an ordinary LED.
- UART for serial communication.
- LED-strip support for an addressable RGB LED.
- Logging for diagnostic messages.

### Step 2 — Find its configuration options

Use either:

- **`menuconfig`**: search for the feature and read its help.
- **The source files**: inspect the relevant Kconfig definitions.

Common starting points:

| Feature | Kconfig location |
|---|---|
| GPIO | `zephyr/drivers/gpio/Kconfig` |
| UART / serial | `zephyr/drivers/serial/Kconfig` |
| Addressable LED strips | `zephyr/drivers/led_strip/Kconfig` |
| PWM | `zephyr/drivers/pwm/Kconfig` |
| I2C | `zephyr/drivers/i2c/Kconfig` |
| SPI | `zephyr/drivers/spi/Kconfig` |
| Logging | `zephyr/subsys/logging/Kconfig` |
| Console | `zephyr/drivers/console/Kconfig` |

A feature's options may be spread across several included files. Hardware-specific
options often have separate files, such as `Kconfig.esp32`, `Kconfig.stm32`, or
`Kconfig.ws2812`.

### Step 3 — Read the option carefully

Check:

- **Symbol name** — the option's identifier.
- **Type** — Boolean, integer, hexadecimal, or string.
- **Default** — the value used when applicable.
- **Dependencies** — conditions required to enable it.
- **Selected symbols** — options enabled automatically.
- **Help text** — purpose and limitations.

A symbol named `SERIAL` becomes `CONFIG_SERIAL` when used in `prj.conf`.

> **Do not assume `default y` means always enabled. Dependencies still apply.**

### Step 4 — Add necessary settings to `prj.conf`

Keep your application's requirements here.

- Do not edit Zephyr's driver Kconfig files just to enable a feature.
- Do not copy every available option.
- Some options are already enabled by board defaults or selected automatically.
- Do not manually force generated `DT_HAS_...` symbols; configure the
  corresponding Devicetree hardware instead.

### Step 5 — Check whether an overlay is needed

Hardware features commonly need Devicetree settings:

| Feature | Typical hardware details |
|---|---|
| External LED | GPIO controller, pin number, polarity |
| UART | Enabled instance, TX/RX pin routing, baud rate |
| Addressable RGB LED | Driver binding, data output routing, pixel count, color order |

Software-only features, such as basic logging, may need only Kconfig settings,
although their backends can require hardware configuration.

### Step 6 — Build and verify

Inspect the generated files:

- **`build/zephyr/.config`** — final software configuration.
- **`build/zephyr/zephyr.dts`** — final hardware description.

Treat these as verification outputs, not permanent files to edit.

### Step 7 — Use the feature in application code

Include the matching header and use the API:

```c
#include <zephyr/drivers/gpio.h>       /* GPIO */
#include <zephyr/drivers/led_strip.h>  /* RGB LED strip */
#include <zephyr/drivers/uart.h>       /* UART */
```

---

## How to find the Kconfig for a used SW/HW module

This is the part most people get stuck on. Follow these steps.

### Step A — Find the `compatible` string in the board Devicetree

Open the board `.dts`:

```text
zephyr/boards/<vendor>/<board>/<board>.dts
```

Look for the device node:

```dts
rgb_led: ws2812@0 {
    compatible = "worldsemi,ws2812-i2s";
    chain-length = <1>;
};
```

The `compatible` string is the key:

```text
worldsemi,ws2812-i2s
```

### Step B — Find the Devicetree binding

```bash
find zephyr/dts/bindings -iname "*ws2812-i2s*"
```

The binding file describes the node's properties:

```text
zephyr/dts/bindings/led_strip/worldsemi,ws2812-i2s.yaml
```

### Step C — Find the driver that handles the `compatible`

```bash
grep -rn "worldsemi,ws2812-i2s" zephyr/drivers
```

This points to the driver source, for example:

```text
zephyr/drivers/led_strip/ws2812_i2s.c
```

### Step D — Find the driver's Kconfig

```bash
grep -rn "WS2812" zephyr/drivers/led_strip/Kconfig*
```

Example result:

```kconfig
config WS2812_STRIP_I2S
    bool "WS2812 LED strip I2S driver"
    default y
    depends on DT_HAS_WORLDSEMI_WS2812_I2S_ENABLED
    select I2S if $(dt_compat_on_bus,$(DT_COMPAT_WORLDSEMI_WS2812_I2S),i2s)
```

What this tells you:

- The driver symbol is `WS2812_STRIP_I2S`.
- It is enabled by default **when** the Devicetree node exists.
- It needs the `I2S` subsystem if the node is on an I2S bus.
- `DT_HAS_WORLDSEMI_WS2812_I2S_ENABLED` is generated automatically from the
  `compatible` string — **never set it manually**.

### Step E — Find the subsystem API symbol

The driver is often automatic, but the **API subsystem** usually must be enabled
explicitly.

```bash
grep -rn "menuconfig LED_STRIP\|config LED_STRIP" zephyr/drivers/led_strip/Kconfig
```

Result:

```kconfig
menuconfig LED_STRIP
    bool "LED strip drivers"
```

So you add:

```conf
CONFIG_LED_STRIP=y
```

### Step F — Check the board defaults

Boards often enable common features already:

```text
zephyr/boards/<vendor>/<board>/<board>_defconfig
```

Example for the ESP32-S3 Mini:

```conf
CONFIG_CONSOLE=y
CONFIG_SERIAL=y
CONFIG_UART_CONSOLE=y
CONFIG_GPIO=y
CONFIG_CLOCK_CONTROL=y
```

That is why `printk()` works over USB-C without extra configuration.

---

## Worked examples

### Example 1 — Plain GPIO LED (Black Pill F401CC)

**Devicetree:**

```dts
leds {
    compatible = "gpio-leds";
    user_led: led {
        gpios = <&gpioc 13 GPIO_ACTIVE_LOW>;
    };
};
aliases { led0 = &user_led; };
```

**Find the Kconfig:**

```bash
grep -rn "config GPIO" zephyr/drivers/gpio/Kconfig
```

**Add to `prj.conf`:**

```conf
CONFIG_GPIO=y
```

**Code:**

```c
#include <zephyr/drivers/gpio.h>
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
```

---

### Example 2 — Addressable RGB LED (ESP32-S3 Mini)

**Devicetree:**

```dts
aliases { rgb0 = &rgb_led; };

&i2s0 {
    status = "okay";
    rgb_led: ws2812@0 {
        compatible = "worldsemi,ws2812-i2s";
        chain-length = <1>;
        color-mapping = <LED_COLOR_ID_GREEN
                         LED_COLOR_ID_RED
                         LED_COLOR_ID_BLUE>;
    };
};
```

**Find the Kconfig:**

```bash
grep -rn "config LED_STRIP" zephyr/drivers/led_strip/Kconfig
grep -rn "WS2812" zephyr/drivers/led_strip/Kconfig*
```

**Add to `prj.conf`:**

```conf
CONFIG_LED_STRIP=y
```

The `WS2812_STRIP_I2S` driver is selected automatically because the Devicetree
node exists.

**Code:**

```c
#include <zephyr/drivers/led_strip.h>
#define STRIP_NODE DT_ALIAS(rgb0)
static const struct device *strip = DEVICE_DT_GET(STRIP_NODE);
```

---

### Example 3 — UART

**Devicetree:**

```dts
&uart0 {
    status = "okay";
    current-speed = <115200>;
    pinctrl-0 = <&uart0_default>;
    pinctrl-names = "default";
};

chosen {
    zephyr,console = &usb_serial;
};
```

**Find the Kconfig:**

```bash
grep -rn "config SERIAL" zephyr/drivers/serial/Kconfig
grep -rn "config UART_ESP32" zephyr/drivers/serial/Kconfig.esp32
```

**Add to `prj.conf`:**

```conf
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y
```

The board defconfig already sets these, so you normally do not need to repeat
them unless you want to make the requirement explicit.

**Code:**

```c
#include <zephyr/drivers/uart.h>
const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));
```

For logging over UART:

```conf
CONFIG_LOG=y
CONFIG_LOG_BACKEND_UART=y
```

---

### Example 4 — Logging

**Find the Kconfig:**

```bash
grep -rn "menuconfig LOG\b" zephyr/subsys/logging/Kconfig
```

**Add to `prj.conf`:**

```conf
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
```

**Code:**

```c
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(my_app, LOG_LEVEL_INF);

LOG_INF("Application started");
```

Logging itself is software-only, but its **backend** may need hardware (UART,
USB, or a memory buffer).

---

## Important rules

1. **Do not edit Zephyr's Kconfig files** to enable a feature. Put your choices
   in `prj.conf`.
2. **Do not force `DT_HAS_...` symbols.** They are generated from Devicetree.
   Fix the Devicetree node instead.
3. **`default y` is not "always on".** Dependencies such as
   `depends on DT_HAS_..._ENABLED` must also be satisfied.
4. **Board defconfigs already enable many things.** Check the board's
   `_defconfig` before adding duplicate lines.
5. **Driver symbols vs. subsystem symbols.**
   - `CONFIG_LED_STRIP` enables the API subsystem.
   - `CONFIG_WS2812_STRIP_I2S` enables the specific driver (often automatic).
6. **Verify, do not guess.** After building, check `.config` and `zephyr.dts`.

---

## Quick checklist

Before using a feature:

- [ ] Identify the subsystem and API.
- [ ] Find its Kconfig options.
- [ ] Read defaults, help, and dependencies.
- [ ] Add necessary settings to `prj.conf`.
- [ ] Configure hardware in an overlay if required.
- [ ] Build and verify the generated configuration.
- [ ] Initialize and use the feature in application code.

---

## Command cheat sheet

```bash
# Interactive search
west build -t menuconfig -d build_<board>
west build -t guiconfig -d build_<board>

# Find a symbol in the source
grep -rn "config LED_STRIP" zephyr/drivers/led_strip/Kconfig
grep -rn "config UART"      zephyr/drivers/serial/Kconfig*
grep -rn "config GPIO"      zephyr/drivers/gpio/Kconfig

# Find the driver for a compatible string
grep -rn "worldsemi,ws2812-i2s" zephyr/drivers
find zephyr/dts/bindings -iname "*ws2812-i2s*"

# Find the board defaults
cat zephyr/boards/<vendor>/<board>/<board>_defconfig

# Verify the build
grep CONFIG_LED_STRIP build_<board>/zephyr/.config
grep CONFIG_SERIAL    build_<board>/zephyr/.config
grep -n "rgb0\|led0\|uart" build_<board>/zephyr/zephyr.dts

# Search the generated Kconfig header
grep -n "LED_STRIP" build_<board>/zephyr/include/generated/zephyr/autoconf.h
```

---

## Related documents

- [Create a New Zephyr Project](../03_Create_New_Project/README.md) — full
  project examples.
- [Devicetree Overlays Guide](<../04_Kconfig_vs_Devicetree(DTS)/Devicetree_Overlays_Guide.md>)
  — where to find overlay properties and how to write overlays.
- [Using Drivers and Software Modules](../03_Create_New_Project/Using_Drivers_And_Software_Modules.md)
  — the general lifecycle for using any driver, subsystem, or module.
- [Kconfig deep dive](README.md) — symbols, `depends on`, `select`, `default`,
  priority hierarchy.
