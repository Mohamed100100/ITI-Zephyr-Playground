# Create a New Zephyr Project — Full Step-by-Step Guide

This guide teaches you **how to go from a board to a working LED project** and
explains every file and every line of code.

It contains two complete examples:

- **Example A — WeAct ESP32-S3 Mini:** on-board **RGB (WS2812) LED**
- **Example B — WeAct Black Pill F401CC:** on-board **plain GPIO LED**

The important part is not the two examples themselves — it is the **recipe** in
Section 1. Once you understand the recipe, you can create a blink project for
almost any board.

> **Companion guides:**
> - [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md)
>   — how to find Kconfig options and drivers for any hardware or software module.
> - [Devicetree Overlays Guide](<../04_Kconfig_vs_Devicetree(DTS)/Devicetree_Overlays_Guide.md>)
>   — where to find overlay properties and how to write overlays.
> - [Using Drivers and Software Modules](Using_Drivers_And_Software_Modules.md)
>   — the general lifecycle for using any driver, subsystem, or module.
>
> **Learning path:** `03 Create Project` → `04 Kconfig vs Devicetree` →
> `05 Kconfig deep dive` → `06 Custom Module`.

---

## Table of Contents

1. [The general recipe (works for any board)](#1-the-general-recipe-works-for-any-board)
2. [Example A — ESP32-S3 Mini RGB LED](#2-example-a--esp32-s3-mini-rgb-led)
3. [Example B — Black Pill F401CC GPIO LED](#3-example-b--black-pill-f401cc-gpio-led)
4. [How to find Kconfig names and drivers](#4-how-to-find-kconfig-names-and-drivers)
5. [How to adapt to a new board](#5-how-to-adapt-to-a-new-board)
6. [Customizing](#6-customizing)
7. [Troubleshooting](#7-troubleshooting)
8. [Cheat sheet](#8-cheat-sheet)

---

## 1. The general recipe (works for any board)

Every Zephyr LED project follows the same 8 steps.

### Step 1 — Find the board target name

```bash
west boards | grep -i <part_of_the_name>
```

Example:

```bash
west boards | grep -i esp32s3
west boards | grep -i blackpill
```

The board name is what you pass to `west build -b <board>`.

For multi-core boards you also need the SoC and core, for example:

```
weact_esp32s3_mini/esp32s3/procpu
blackpill_f401cc
```

The board files live in:

```
zephyr/boards/<vendor>/<board>/
```

Example:

```
zephyr/boards/weact/esp32s3_mini/
zephyr/boards/weact/blackpill_f401cc/
```

---

### Step 2 — Open the board Devicetree

The `.dts` file tells you **what hardware exists** and **which pin is the LED**.

```bash
zephyr/boards/<vendor>/<board>/<board>.dts
```

Look for three things:

1. **`aliases`** — the short names your code uses.
2. **`compatible`** — tells Zephyr which driver handles the device.
3. **`gpios` / `chain-length` / other properties** — the actual wiring.

Example from the ESP32-S3 Mini:

```dts
aliases {
    rgb0 = &rgb_led;
};

&i2s0 {
    rgb_led: ws2812@0 {
        compatible = "worldsemi,ws2812-i2s";
        chain-length = <1>;
        color-mapping = <LED_COLOR_ID_GREEN
                         LED_COLOR_ID_RED
                         LED_COLOR_ID_BLUE>;
    };
};
```

This tells us:

- The LED is available through the alias **`rgb0`**.
- It is a **WS2812** LED (not a plain GPIO LED).
- There is **1** LED in the chain.
- The color order is **GREEN, RED, BLUE**.

Example from the Black Pill:

```dts
aliases {
    led0 = &user_led;
};

leds {
    compatible = "gpio-leds";

    user_led: led {
        gpios = <&gpioc 13 GPIO_ACTIVE_LOW>;
    };
};
```

This tells us:

- The LED is available through the alias **`led0`**.
- It is a **plain GPIO LED**.
- It is on **port C, pin 13**, and it is **active-low**.

After a build, you can also inspect the final merged Devicetree:

```bash
grep -n "rgb0\|led0" build/zephyr/zephyr.dts
```

---

### Step 3 — Choose the right driver and API

The `compatible` string tells you which Zephyr API to use.

| Hardware | Devicetree sign | C API header | Kconfig to enable |
|---|---|---|---|
| Plain GPIO LED | `compatible = "gpio-leds"` | `zephyr/drivers/gpio.h` | `CONFIG_GPIO=y` |
| WS2812 / NeoPixel | `compatible = "worldsemi,ws2812-*"` | `zephyr/drivers/led_strip.h` | `CONFIG_LED_STRIP=y` |
| PWM RGB LED | `compatible = "pwm-leds"` | `zephyr/drivers/pwm.h` | `CONFIG_PWM=y` |
| I2C device | `compatible = "vendor,device"` on an I2C bus | `zephyr/drivers/i2c.h` | `CONFIG_I2C=y` |
| SPI device | `compatible = "vendor,device"` on an SPI bus | `zephyr/drivers/spi.h` | `CONFIG_SPI=y` |

**Rule:** never hard-code pin numbers in C. Always get the pin/device from the
Devicetree with macros such as `DT_ALIAS()`, `GPIO_DT_SPEC_GET()`, and
`DEVICE_DT_GET()`.

---

### Step 4 — Find the Kconfig names

Kconfig symbols always start with `CONFIG_`.

You can find them in four ways:

**A. Search the source code**

```bash
grep -rn "config LED_STRIP" zephyr/drivers/led_strip/Kconfig
grep -rn "config GPIO"      zephyr/drivers/gpio/Kconfig
grep -rn "config PWM"       zephyr/drivers/pwm/Kconfig
```

**B. Use `menuconfig`** (interactive menu):

```bash
west build -t menuconfig -d build_esp32s3
```

Use `/` to search inside the menu.

**C. Use `guiconfig`** (graphical menu):

```bash
west build -t guiconfig -d build_esp32s3
```

**D. Look at the final generated config** after a build:

```bash
grep CONFIG_LED_STRIP build_esp32s3/zephyr/.config
grep CONFIG_GPIO     build_esp32s3/zephyr/.config
```

---

### Step 5 — Create the project

A Zephyr app only needs three files:

```
my_app/
├── CMakeLists.txt
├── prj.conf
└── src/
    └── main.c
```

```bash
mkdir -p my_app/src
```

---

### Step 6 — Write the code

- `CMakeLists.txt` says what to build.
- `prj.conf` enables the drivers/features.
- `main.c` uses the Devicetree macros to talk to the hardware.

---

### Step 7 — Build

**Important:** `west build` must run from inside the Zephyr workspace. The app
itself can live outside the workspace — just pass its path.

```bash
cd <your-zephyr-workspace>
west build -p always -b <board> -d <build_dir> <path-to-app>
```

Example:

```bash
cd /media/gemy/Linux_Workspace/ITI/Zypher_WS/zephyrproject
west build -p always -b weact_esp32s3_mini/esp32s3/procpu \
  -d build_esp32s3 \
  ../ITI-Zephyr-Playground/03_Create_New_Project/esp32s3_mini_blinky
```

- `-p always` = clean build
- `-b` = board target
- `-d` = build directory (optional, default is `build`)
- last argument = application folder

---

### Step 8 — Flash and monitor

```bash
west flash -d <build_dir>
```

For ESP32 boards:

```bash
west espressif monitor
# or
python -m serial.tools.miniterm /dev/ttyACM0 115200
```

For STM32 Black Pill, use the DFU bootloader (Section 3.7).

---

## 2. Example A — ESP32-S3 Mini RGB LED

**Project location in this repo:**

```
03_Create_New_Project/esp32s3_mini_blinky/
├── CMakeLists.txt
├── prj.conf
└── src/
    └── main.c
```

### 2.1 Board facts

| Item | Value |
|---|---|
| Board | WeAct Studio ESP32-S3-Mini |
| Zephyr target | `weact_esp32s3_mini/esp32s3/procpu` |
| CPU | ESP32-S3, dual-core, 240 MHz |
| Flash / PSRAM | 4 MB / 2 MB |
| On-board LED | WS2812 RGB LED on **GPIO48** |
| LED Devicetree alias | **`rgb0`** |
| LED compatible | `worldsemi,ws2812-i2s` |
| Console | USB Serial/JTAG over the USB-C port |
| Button | BOOT button on GPIO0, alias `sw0` |

### 2.2 Why a normal `blinky` does not work here

The standard Zephyr `samples/basic/blinky` expects a **plain GPIO LED** with the
alias `led0` and `compatible = "gpio-leds"`.

The ESP32-S3 Mini has **no `led0` alias**. Its LED is a **WS2812 addressable RGB
LED**, which needs the **LED strip** driver. That is why we use
`led_strip_update_rgb()` instead of `gpio_pin_toggle()`.

This is exactly the kind of thing you discover in **Step 2** (read the board
`.dts`) and **Step 3** (choose the API).

### 2.3 `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(esp32s3_mini_blinky)

target_sources(app PRIVATE src/main.c)
```

**Line by line:**

| Line | What it does |
|---|---|
| `cmake_minimum_required(VERSION 3.28.0)` | Zephyr 4.4 needs CMake 3.28 or newer. |
| `find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})` | Loads the Zephyr build system. `ZEPHYR_BASE` points to the Zephyr source. |
| `project(esp32s3_mini_blinky)` | Gives the application a name. |
| `target_sources(app PRIVATE src/main.c)` | Adds `src/main.c` to the application. Add more `.c` files here as the app grows. |

### 2.4 `prj.conf`

```conf
CONFIG_LED_STRIP=y
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
CONFIG_PRINTK=y
```

**Line by line:**

| Line | What it does | How we found it |
|---|---|---|
| `CONFIG_LED_STRIP=y` | Enables the LED strip API and the WS2812 driver. | `zephyr/drivers/led_strip/Kconfig` |
| `CONFIG_LOG=y` | Enables the logging subsystem. | `zephyr/subsys/logging/Kconfig` |
| `CONFIG_LOG_DEFAULT_LEVEL=3` | Sets the default log level to info. | `zephyr/subsys/logging/Kconfig` |
| `CONFIG_PRINTK=y` | Enables `printk()` output. | `zephyr/subsys/debug/Kconfig` |

**Why only `CONFIG_LED_STRIP=y`?**

The board's Devicetree already contains the WS2812 node:

```dts
compatible = "worldsemi,ws2812-i2s";
```

Zephyr automatically generates a Kconfig symbol for that node, and the driver
Kconfig says:

```kconfig
config WS2812_STRIP_I2S
    bool "WS2812 LED strip I2S driver"
    default y
    depends on DT_HAS_WORLDSEMI_WS2812_I2S_ENABLED
```

So the driver is **automatically enabled** because the Devicetree node exists.
You only need to enable the high-level API: `CONFIG_LED_STRIP=y`.

This is a very common Zephyr pattern:

```
Devicetree node exists  →  driver Kconfig becomes available (often default y)
You enable              →  the subsystem API (GPIO, LED_STRIP, PWM, I2C...)
```

### 2.5 `src/main.c`

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

/* The WeAct ESP32-S3-Mini exposes its on-board WS2812 RGB LED
 * through the "rgb0" devicetree alias.
 */
#define STRIP_NODE DT_ALIAS(rgb0)

#if !DT_NODE_EXISTS(STRIP_NODE)
#error "Board does not define an 'rgb0' alias"
#endif

#define STRIP_LEN DT_PROP(STRIP_NODE, chain_length)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[STRIP_LEN];

static const struct led_rgb colors[] = {
	RGB(0x20, 0x00, 0x00), /* red */
	RGB(0x00, 0x20, 0x00), /* green */
	RGB(0x00, 0x00, 0x20), /* blue */
	RGB(0x00, 0x00, 0x00), /* off */
};

int main(void)
{
	if (!device_is_ready(strip)) {
		printk("RGB LED strip device is not ready\n");
		return 0;
	}

	printk("ESP32-S3 Mini blinky started\n");

	while (1) {
		for (size_t color = 0; color < ARRAY_SIZE(colors); color++) {
			for (size_t i = 0; i < STRIP_LEN; i++) {
				pixels[i] = colors[color];
			}

			int ret = led_strip_update_rgb(strip, pixels, STRIP_LEN);

			if (ret < 0) {
				printk("led_strip_update_rgb failed: %d\n", ret);
				return 0;
			}

			k_msleep(500);
		}
	}

	return 0;
}
```

### 2.6 Line-by-line explanation

#### Includes

```c
#include <zephyr/kernel.h>
```
Gives you kernel functions such as `k_msleep()`.

```c
#include <zephyr/device.h>
```
Gives you `DEVICE_DT_GET()` and `device_is_ready()`.

```c
#include <zephyr/drivers/led_strip.h>
```
Gives you the LED strip API: `struct led_rgb`, `led_strip_update_rgb()`.

```c
#include <zephyr/sys/printk.h>
```
Gives you `printk()`.

```c
#include <zephyr/sys/util.h>
```
Gives you `ARRAY_SIZE()`.

#### Finding the LED in the Devicetree

```c
#define STRIP_NODE DT_ALIAS(rgb0)
```
`DT_ALIAS(rgb0)` asks the Devicetree: "give me the node that the alias `rgb0`
points to". On this board that is the WS2812 LED node.

The alias comes from the board `.dts`:

```dts
aliases {
    rgb0 = &rgb_led;
};
```

#### Compile-time safety check

```c
#if !DT_NODE_EXISTS(STRIP_NODE)
#error "Board does not define an 'rgb0' alias"
#endif
```
If you build this app for a board that has no `rgb0` alias, the build stops
immediately with a clear message. This is much easier to debug than a confusing
linker error.

#### Number of LEDs

```c
#define STRIP_LEN DT_PROP(STRIP_NODE, chain_length)
```
`chain_length` comes from the board Devicetree. Here it is `1`, because the
board has one RGB LED.

#### Color helper

```c
#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }
```
This is a small macro that makes color definitions shorter.

`0x20` is the brightness (0–255). `0x20` is about 12.5%, which is bright enough
but does not draw too much current.

#### Device pointer and pixel buffer

```c
static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[STRIP_LEN];
```
- `DEVICE_DT_GET()` returns the driver instance for the LED strip.
- `pixels[]` is the buffer you fill with colors before sending them to the LED.

#### Color list

```c
static const struct led_rgb colors[] = {
	RGB(0x20, 0x00, 0x00), /* red */
	RGB(0x00, 0x20, 0x00), /* green */
	RGB(0x00, 0x00, 0x20), /* blue */
	RGB(0x00, 0x00, 0x00), /* off */
};
```
The app cycles through red, green, blue, and off.

#### `main()` — the starting point

```c
int main(void)
{
```
`main()` is called by Zephyr after the kernel and drivers are initialized.

#### Check that the driver is ready

```c
	if (!device_is_ready(strip)) {
		printk("RGB LED strip device is not ready\n");
		return 0;
	}
```
Drivers are initialized during boot. This check makes sure the LED strip driver
initialized correctly before we use it.

#### Print a message

```c
	printk("ESP32-S3 Mini blinky started\n");
```
This appears on the USB-C serial console.

#### Main loop

```c
	while (1) {
```
An embedded program normally never exits, so the loop runs forever.

#### Loop over colors

```c
		for (size_t color = 0; color < ARRAY_SIZE(colors); color++) {
```
`ARRAY_SIZE(colors)` is `4`, so `color` goes `0, 1, 2, 3`.

#### Fill every pixel with the current color

```c
			for (size_t i = 0; i < STRIP_LEN; i++) {
				pixels[i] = colors[color];
			}
```
This board has one pixel, so this loop runs once. The loop makes the code work
on a longer strip too.

#### Send the colors to the LED

```c
			int ret = led_strip_update_rgb(strip, pixels, STRIP_LEN);
```
This is the important call. It sends the `pixels` buffer to the WS2812 driver,
which generates the correct timing signal on GPIO48.

#### Error check

```c
			if (ret < 0) {
				printk("led_strip_update_rgb failed: %d\n", ret);
				return 0;
			}
```
If the driver reports an error, print it and stop.

#### Wait

```c
			k_msleep(500);
```
Sleep for 500 ms. The CPU can do other work (or idle) during this time. The full
cycle red → green → blue → off takes 2 seconds.

### 2.7 How the Devicetree pieces connect

```
main.c
  DT_ALIAS(rgb0)
        │
        ▼
board .dts
  aliases { rgb0 = &rgb_led; }
        │
        ▼
  rgb_led: ws2812@0 {
      compatible = "worldsemi,ws2812-i2s";
      chain-length = <1>;
  }
        │
        ▼
driver: zephyr/drivers/led_strip/ws2812_i2s.c
        │
        ▼
hardware: GPIO48 → WS2812 RGB LED
```

You never write `GPIO48` in your code. The board Devicetree knows the pin; your
code only knows the alias `rgb0`. That is why the same code can be reused.

### 2.8 Build

**Important:** run `west build` from the Zephyr workspace, not from the app
folder. West needs the workspace to find Zephyr.

> The paths below match this machine. Replace them with your own workspace and
> repo locations if yours are different.

```bash
# 1. Activate the Python environment
source /media/gemy/Linux_Workspace/ITI/Zypher_WS/zephyrproject/.venv/bin/activate

# 2. Go to the Zephyr workspace
cd /media/gemy/Linux_Workspace/ITI/Zypher_WS/zephyrproject

# 3. Build the app
west build -p always -b weact_esp32s3_mini/esp32s3/procpu \
  -d build_esp32s3 \
  ../ITI-Zephyr-Playground/03_Create_New_Project/esp32s3_mini_blinky
```

The firmware is created in:

```
build_esp32s3/zephyr/zephyr.elf
build_esp32s3/zephyr/zephyr.bin
```

### 2.9 Flash

```bash
west flash -d build_esp32s3
```

If there are several serial ports, choose the ESP32 one explicitly:

```bash
west flash -d build_esp32s3 --esp-device /dev/ttyACM0
```

The board uses the **USB Serial/JTAG** interface, so the same USB-C cable is
used for both flashing and the console. Usually you do not need to press BOOT0.

If flashing fails, enter download mode manually:

1. Hold **BOOT**.
2. Press and release **RESET**.
3. Release **BOOT**.
4. Run `west flash -d build_esp32s3` again.

### 2.10 Monitor the console

```bash
west espressif monitor
```

or:

```bash
python -m serial.tools.miniterm /dev/ttyACM0 115200
```

You should see:

```
ESP32-S3 Mini blinky started
```

### 2.11 Expected result

The on-board RGB LED cycles:

```
red → green → blue → off → red → ...
```

Each color stays on for **500 ms**.

---

## 3. Example B — Black Pill F401CC GPIO LED

This is the classic Zephyr blinky, but for a **plain GPIO LED**.

### 3.1 Board facts

| Item | Value |
|---|---|
| Board | WeAct Black Pill V1.2 |
| Zephyr target | `blackpill_f401cc` |
| CPU | STM32F401CCU6, Cortex-M4F, 84 MHz |
| Flash / SRAM | 256 KB / 64 KB |
| On-board LED | PC13, **active-low** |
| LED alias | **`led0`** |
| LED compatible | `gpio-leds` |
| Console | USART1: PA9 (TX), PA10 (RX), 115200 |
| Flashing | USB DFU bootloader |

### 3.2 Project files

```
03_Create_New_Project/blackpill_blinky/
├── CMakeLists.txt
├── prj.conf
└── src/
    └── main.c
```

### 3.3 `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blackpill_blinky)

target_sources(app PRIVATE src/main.c)
```

Same structure as Example A.

### 3.4 `prj.conf`

```conf
CONFIG_GPIO=y
```

**Why?**

The LED is a plain GPIO LED, so we use the GPIO API. `CONFIG_GPIO=y` enables the
GPIO subsystem. The STM32 GPIO driver is selected automatically because the
board Devicetree has a GPIO controller node.

### 3.5 `src/main.c`

```c
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Board does not define a usable 'led0' alias"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return 0;
	}

	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}

		k_msleep(500);
	}

	return 0;
}
```

### 3.6 Line-by-line explanation

| Code | Meaning |
|---|---|
| `#include <zephyr/kernel.h>` | Kernel functions, `k_msleep()`. |
| `#include <zephyr/drivers/gpio.h>` | GPIO API. |
| `DT_ALIAS(led0)` | Finds the LED node in the Devicetree. |
| `DT_NODE_HAS_STATUS(..., okay)` | Stops the build if the board has no usable `led0`. |
| `GPIO_DT_SPEC_GET(...)` | Creates a `gpio_dt_spec` with port, pin, and flags. |
| `gpio_is_ready_dt()` | Checks the GPIO driver is ready. |
| `gpio_pin_configure_dt(..., GPIO_OUTPUT_INACTIVE)` | Sets the pin as output, initially off. |
| `gpio_pin_toggle_dt()` | Flips the LED on/off. |
| `k_msleep(500)` | Waits 500 ms. |

The board Devicetree says the LED is **active-low**. Zephyr handles that
automatically, so `gpio_pin_toggle_dt()` works correctly without extra code.

### 3.7 Build and flash

> Replace the paths below with your own workspace and repo locations.

```bash
source /media/gemy/Linux_Workspace/ITI/Zypher_WS/zephyrproject/.venv/bin/activate
cd /media/gemy/Linux_Workspace/ITI/Zypher_WS/zephyrproject

west build -p always -b blackpill_f401cc \
  -d build_blackpill \
  ../ITI-Zephyr-Playground/03_Create_New_Project/blackpill_blinky

west flash -d build_blackpill
```

To flash, the board must be in DFU mode:

1. Hold **BOOT0**.
2. Press and release **NRST**.
3. Release **BOOT0**.
4. Run `west flash -d build_blackpill`.

After flashing, press **NRST** once. The LED blinks once per second.

---

## 4. How to find Kconfig names and drivers

The short version: **the Kconfig symbol usually matches the subsystem name**,
and you can confirm it with `menuconfig` or a source search.

| Feature | Typical Kconfig symbol |
|---|---|
| GPIO | `CONFIG_GPIO` |
| LED strip | `CONFIG_LED_STRIP` |
| PWM | `CONFIG_PWM` |
| I2C | `CONFIG_I2C` |
| SPI | `CONFIG_SPI` |
| UART/serial | `CONFIG_SERIAL` |
| Logging | `CONFIG_LOG` |
| Console | `CONFIG_CONSOLE` |

Three quick ways to confirm a symbol:

1. **Search the source**
   ```bash
   grep -rn "config LED_STRIP" zephyr/drivers/led_strip/Kconfig
   ```
2. **Search menuconfig**
   ```bash
   west build -t menuconfig -d build_esp32s3
   ```
   Press `/`, type the feature name, then Enter.
3. **Search the generated config after a build**
   ```bash
   grep CONFIG_LED_STRIP build_esp32s3/zephyr/.config
   ```

Devicetree often enables the driver automatically: a node with
`compatible = "worldsemi,ws2812-i2s"` generates
`DT_HAS_WORLDSEMI_WS2812_I2S_ENABLED`, which the driver's Kconfig uses as a
dependency. That is why you usually only enable the subsystem API
(`CONFIG_LED_STRIP=y`, `CONFIG_GPIO=y`, `CONFIG_PWM=y`).

> **Full guide:** [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md)
> covers the three configuration layers, four search methods, and how to trace a
> `compatible` string to its driver.

---

## 5. How to adapt to a new board

Use this checklist every time:

1. **Find the board target**
   ```bash
   west boards | grep -i <name>
   ```
2. **Open the board `.dts`**
   ```
   zephyr/boards/<vendor>/<board>/<board>.dts
   ```
3. **Find the LED in `aliases` and `leds`**
   - `led0` → GPIO LED
   - `rgb0` or `led-strip` → addressable RGB LED
   - `pwm-leds` → PWM LED
4. **Pick the API from the `compatible` string**

   | `compatible` | API | Kconfig |
   |---|---|---|
   | `gpio-leds` | GPIO | `CONFIG_GPIO=y` |
   | `worldsemi,ws2812-*` | LED strip | `CONFIG_LED_STRIP=y` |
   | `pwm-leds` | PWM | `CONFIG_PWM=y` |

5. **Write `prj.conf`** with the matching `CONFIG_...=y`.
6. **Write `main.c`** with `DT_ALIAS(...)` and the matching API.
7. **Build from the Zephyr workspace**
   ```bash
   west build -p always -b <board> -d build_<name> <path-to-app>
   ```
8. **Flash and monitor**
   ```bash
   west flash -d build_<name>
   ```
9. **If something is missing**, inspect:
   - `build_<name>/zephyr/zephyr.dts`
   - `build_<name>/zephyr/.config`
   - `west build -t menuconfig -d build_<name>`

If the board has a plain `led0` and `gpio-leds`, the Black Pill code from
Section 3 works unchanged. If it has `rgb0` and `worldsemi,ws2812-*`, the
ESP32-S3 code from Section 2 works with `DT_ALIAS(rgb0)`.

> **Need to change a pin or add an external LED?**
> See [Devicetree Overlays Guide](<../04_Kconfig_vs_Devicetree(DTS)/Devicetree_Overlays_Guide.md>).

---

## 6. Customizing

### 6.1 Change the ESP32-S3 colors

```c
static const struct led_rgb colors[] = {
	RGB(0x20, 0x00, 0x00), /* red */
	RGB(0x20, 0x10, 0x00), /* orange */
	RGB(0x20, 0x20, 0x00), /* yellow */
	RGB(0x00, 0x20, 0x00), /* green */
	RGB(0x00, 0x00, 0x20), /* blue */
	RGB(0x10, 0x00, 0x20), /* purple */
	RGB(0x00, 0x00, 0x00), /* off */
};
```

### 6.2 Change the speed

```c
k_msleep(100);   /* faster */
k_msleep(1000);  /* slower */
```

### 6.3 Change the brightness

```c
RGB(0x40, 0x00, 0x00)   /* brighter red */
RGB(0x08, 0x00, 0x00)   /* dimmer red */
```

Values range from `0x00` (off) to `0xFF` (maximum).

### 6.4 Add UART logging

`prj.conf`:

```conf
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
```

`main.c`:

```c
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(my_app, LOG_LEVEL_INF);

LOG_INF("Application started");
```

### 6.5 Read the BOOT button on the ESP32-S3 Mini

The board defines:

```dts
aliases {
    sw0 = &button0;
};
```

Code:

```c
#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

gpio_pin_configure_dt(&button, GPIO_INPUT);
bool pressed = gpio_pin_get_dt(&button) > 0;
```

This requires `CONFIG_GPIO=y` in `prj.conf`.

### 6.6 Change the Black Pill LED pin with an overlay

Create:

```
blackpill_blinky/boards/blackpill_f401cc.overlay
```

Content:

```dts
&user_led {
    gpios = <&gpioc 14 GPIO_ACTIVE_LOW>;
};
```

This overrides the board's `user_led` pin. `led0` still points to it, so
`DT_ALIAS(led0)` now controls PC14. `main.c` does not change.

---

## 7. Troubleshooting

| Problem | Fix |
|---|---|
| `west: unknown command "build"` | You are outside the West workspace. `cd` into the Zephyr workspace first. |
| `CMake 3.28.0 or higher is required` | Install a newer CMake (for example `pip install cmake` inside the venv). |
| `Board does not define an 'rgb0' alias` | You built for the wrong board, or the board has a different LED alias. Check its `.dts`. |
| `Board does not define a usable 'led0' alias` | The board has no plain GPIO LED. Check for `rgb0` or a WS2812 node instead. |
| `undefined reference to gpio_pin_...` | Add `CONFIG_GPIO=y` to `prj.conf`. |
| `undefined reference to led_strip_...` | Add `CONFIG_LED_STRIP=y` to `prj.conf`. |
| LED driver not ready | Check the Devicetree node status is `okay`, and that the required bus (I2S/SPI/PWM) is enabled. |
| `west flash` cannot find the port | Check `ls /dev/ttyACM*`; use `--esp-device /dev/ttyACM0`. |
| Permission denied on `/dev/ttyACM0` | Add your user to the `dialout` group and log in again. |
| ESP32 does not flash | Hold BOOT, tap RESET, release BOOT, then flash again. |
| LED does not blink | Check the color values are not all zero, and check the serial console for errors. |
| Path with spaces breaks the build | Keep project paths free of spaces. Zephyr/CMake can fail on spaces in the app path. |

---

## 8. Cheat sheet

### ESP32-S3 Mini

```bash
# environment
source /media/gemy/Linux_Workspace/ITI/Zypher_WS/zephyrproject/.venv/bin/activate
cd /media/gemy/Linux_Workspace/ITI/Zypher_WS/zephyrproject

# build
west build -p always -b weact_esp32s3_mini/esp32s3/procpu \
  -d build_esp32s3 \
  ../ITI-Zephyr-Playground/03_Create_New_Project/esp32s3_mini_blinky

# flash
west flash -d build_esp32s3

# monitor
west espressif monitor
# or
python -m serial.tools.miniterm /dev/ttyACM0 115200
```

### Black Pill F401CC

```bash
# build
west build -p always -b blackpill_f401cc \
  -d build_blackpill \
  ../ITI-Zephyr-Playground/03_Create_New_Project/blackpill_blinky

# flash (hold BOOT0, tap NRST, release BOOT0 first)
west flash -d build_blackpill

# serial console (USB-TTL on PA9/PA10)
python -m serial.tools.miniterm /dev/ttyUSB0 115200
```

### Discovery commands

```bash
west boards | grep -i <name>          # find a board
grep -n "aliases" -A10 <board>.dts    # find LED aliases
grep -n "compatible" <board>.dts      # find drivers
grep -rn "config LED_STRIP" zephyr/drivers/led_strip/Kconfig
west build -t menuconfig -d build     # search Kconfig
grep CONFIG_ build/zephyr/.config     # see final config
grep -n "led0\|rgb0" build/zephyr/zephyr.dts
```
