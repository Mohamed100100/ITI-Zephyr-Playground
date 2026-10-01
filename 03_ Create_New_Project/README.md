# Zephyr Project from Scratch — LED Toggle on WeAct Black Pill (STM32F401CC)

> **Board:** WeAct Black Pill V1.2 — STM32F401CCU6 (Cortex-M4F, 84 MHz, 256 KB flash / 64 KB SRAM)
> **Zephyr board target:** `blackpill_f401cc`
> **Goal:** create a brand-new Zephyr application, build it, and flash it so the
> on-board LED (PC13, active-low) blinks/toggles.
>
> **Prerequisite:** a working Zephyr workspace. If you don't have one yet, follow
> the workspace installation guide first (`west init` + `west update` + SDK +
> venv). Every command below assumes:
> ```bash
> source ~/zephyrproject/.venv/bin/activate
> ```

---

## Table of Contents

1. [Know your board](#1-know-your-board)
2. [Create the project](#2-create-the-project)
3. [Write the code](#3-write-the-code)
4. [Build](#4-build)
5. [Flash the Black Pill (DFU — no ST-Link needed)](#5-flash-the-black-pill-dfu--no-st-link-needed)
6. [What you should see](#6-what-you-should-see)
7. [How it works — line-by-line](#7-how-it-works--line-by-line)
8. [Customizing (period, other pins, log output)](#8-customizing)
9. [Alternative: flash via SWD (ST-Link/J-Link)](#9-alternative-flash-via-swd-st-linkj-link)
10. [Troubleshooting](#10-troubleshooting)
11. [Cheat sheet](#11-cheat-sheet)

---

## 1. Know your board

Verify Zephyr knows your board:

```bash
west boards | grep -i blackpill
# blackpill_f401ce, blackpill_f401cc, blackpill_f411ce, blackpill_h523ce, blackpill_u585ci
```

**STM32F401CCU6 facts (from the board's devicetree — `zephyr/boards/weact/blackpill_f401cc/`):**

| Item | Value |
|---|---|
| Zephyr target name | `blackpill_f401cc` |
| LED | `PC13`, **active-low** (sinking), alias `led0` |
| User button | `PA0`, alias `sw0` |
| Console UART | `USART1` TX=`PA9` RX=`PA10`, 115200 8N1 (needs USB-TTL dongle) |
| Clock | 25 MHz HSE + PLL → 84 MHz sysclk |
| Flash via USB | factory ROM DFU bootloader (`0483:df11`) |
| Flash via SWD | 3V3, GND, SWCLK, SWDIO header |

> ⚠️ The USB-C port is **power + DFU only** — it is *not* a UART. To see `printk`
> output you must wire PA9/PA10 to a USB-TTL adapter.

---

## 2. Create the project

A Zephyr application is just a folder with three files: `CMakeLists.txt`,
`prj.conf`, and `src/main.c`.

```bash
cd ~/zephyrproject                      # or anywhere you like
mkdir led_toggle && cd led_toggle
mkdir src
```

Final layout:

```
led_toggle/
├── CMakeLists.txt      ← build system entry point
├── prj.conf            ← Kconfig options for this app
└── src/
    └── main.c          ← the application code
```

### 2.1 `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(led_toggle)

target_sources(app PRIVATE src/main.c)
```

Line by line:

| Line | What it does |
|---|---|
| `cmake_minimum_required(VERSION 3.28.0)` | The minimum CMake version Zephyr requires. |
| `find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})` | Locates Zephyr using the `ZEPHYR_BASE` environment variable (set by `west zephyr-export` / the venv setup) so the build knows where the OS lives. Fails the build if Zephyr isn't found. |
| `project(led_toggle)` | Declares the application name inside the Zephyr build system. |
| `target_sources(app PRIVATE src/main.c)` | Tells the build which source files belong to the application — here, `main.c` in `src/`. Add more files to this list as the app grows. |

### 2.2 `prj.conf`

```conf
CONFIG_GPIO=y
```

That's all this project needs. `CONFIG_GPIO=y` enables the GPIO driver subsystem.
(You can also add `CONFIG_LOG=y` and friends later — see Section 8.)

> Note: this file can be empty for blinky on most boards, because the board's own
> defconfig already enables GPIO. Keeping the explicit line documents the
> requirement and makes the app portable.

---

## 3. Write the code

### 3.1 `src/main.c`

```c
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* The devicetree alias for the on-board LED (PC13, active-low).
 * Defined by the board's .dts file; DT_ALIAS(led0) resolves to its node.
 */
#define LED0_NODE DT_ALIAS(led0)

/* Compile-time check: stop the build early with a clear message if the
 * board doesn't define a led0 alias.
 */
#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Board does not define a usable 'led0' alias in its devicetree"
#endif

/* Expands to a ready-to-use struct with the device pointer, pin number
 * and pin flags (including the GPIO_ACTIVE_LOW flag for this LED).
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
    int ret;

    /* Make sure the GPIO device is ready before touching it. */
    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    /* Configure the pin as output, starting with the LED off
     * (for an active-low LED, "off" = logical 1).
     */
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        return 0;
    }

    while (1) {
        /* Toggle the pin: PC13 sinks current -> LED on,
         * releases -> LED off.
         */
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }
        k_msleep(500);      /* wait 500 ms -> full blink period = 1 s */
    }

    return 0;   /* never reached */
}
```

---

## 4. Build

From the project folder (with the venv activated):

```bash
cd ~/zephyrproject/led_toggle        # your app folder
west build -b blackpill_f401cc .
```

That's it — one command. West/CMake will:

1. Resolve the board's devicetree + Kconfig.
2. Compile Zephyr + the app with the `arm-zephyr-eabi-gcc` toolchain from the SDK.
3. Produce `build/zephyr/zephyr.elf`, `zephyr.hex`, and `zephyr.bin`.

### Useful build variants

```bash
west build -p always -b blackpill_f401cc .   # pristine build (clean first)
west build                                   # incremental rebuild after edits
west build -t menuconfig                     # interactive Kconfig editor
```

---

## 5. Flash the Black Pill (DFU — no ST-Link needed)

The STM32F401 ships with a factory ROM USB DFU bootloader, so you only need a
USB-C cable.

### Step 1 — install dfu-util

```bash
sudo apt install dfu-util
dfu-util --version        # >= 0.8 required
```

### Step 2 — enter DFU mode

1. Plug the board into USB-C (it powers up).
2. **Hold BOOT0**, press and release **NRST**, then release **BOOT0**.
3. The board re-enumerates as `0483:df11 STM32 DFU`.

Verify:

```bash
lsusb | grep -i dfu
# Bus 001 Device 0xx: ID 0483:df11 STMicroelectronics STM Device in DFU Mode
dfu-util -l
```

### Step 3 — flash

```bash
west flash
```

Under the hood this runs the equivalent of:

```bash
dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D build/zephyr/zephyr.bin
```

* `-a 0` — target flash interface AltSetting 0 (internal flash)
* `-s 0x08000000:leave` — write at the start of flash, then leave DFU mode
* `-D build/zephyr/zephyr.bin` — the binary built in step 4

### Step 4 — run

Press **NRST** once after flashing (or `leave` does it automatically) to boot
your app.

---

## 6. What you should see

The green LED on the Black Pill blinks: **on for 500 ms, off for 500 ms** (1 Hz).

> Remember the LED is **active-low** (`GPIO_OUTPUT_INACTIVE` = logical 1 = LED off
> at boot), but `gpio_pin_toggle_dt()` handles polarity for you via the
> devicetree flags — you just toggle.

---

## 7. How it works — line-by-line

| Code | Meaning |
|---|---|
| `#include <zephyr/kernel.h>` | Kernel API: `k_msleep()`, etc. |
| `#include <zephyr/drivers/gpio.h>` | Portable GPIO driver API. **Never** touch HAL/registers directly. |
| `DT_ALIAS(led0)` | Devicetree macro — resolves the `led0` alias to a node. The alias lives in the board's `.dts` (`/ { aliases { led0 = &user_led; }; }`). |
| `DT_NODE_HAS_STATUS(..., okay)` | Compile-time safety: fail the build with a clear error if the board has no usable `led0`. |
| `GPIO_DT_SPEC_GET(LED0_NODE, gpios)` | Expands to `struct gpio_dt_spec` = `{ .port = DEVICE_DT_GET(...), .pin = 13, .dt_flags = GPIO_ACTIVE_LOW }`. All pin knowledge comes from devicetree, **not** hard-coded. |
| `gpio_is_ready_dt(&led)` | Checks the GPIO port driver is initialized. |
| `gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE)` | Sets PC13 as output, initial state "inactive" (LED off for active-low). |
| `gpio_pin_toggle_dt(&led)` | Flips the pin, respecting the active-low polarity automatically. |
| `k_msleep(500)` | Sleep 500 ms — cooperative (the CPU can idle; with no other threads the idle thread runs). |

**Why devicetree macros instead of raw pins?** Because the same code compiles for
any board: on a different board, `led0` may be PA5 active-high — the macros pick
that up automatically. Your code never changes.

---

## 8. Customizing

### 8.1 Change the blink period

```c
k_msleep(100);        // 100 ms on / 100 ms off -> 5 Hz blink
```

### 8.2 Use a different pin (e.g. PC14) via an overlay

Create `led_toggle/app.overlay` (no need to edit board files):

```dts
/ {
    aliases {
        led0 = &my_led;
    };

    my_led: my-led {
        compatible = "gpio-leds";
        gpios = <&gpioc 14 GPIO_ACTIVE_LOW>;
        label = "My LED";
    };
};
```

Rebuild — `DT_ALIAS(led0)` now resolves to PC14. Nothing in `main.c` changes.

### 8.3 Add log output over UART (115200 8N1 on PA9/PA10)

`prj.conf`:

```conf
CONFIG_GPIO=y
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
```

Code — replace the toggle body:

```c
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_toggle, LOG_LEVEL_INF);

/* inside main(), before the loop: */
LOG_INF("LED toggle app starting");

/* inside the loop: */
LOG_INF("LED state toggled");
```

Wire PA9 → RX, PA10 → TX of a USB-TTL dongle (3.3 V!), GND → GND, then:

```bash
pip install pyserial
python -m serial.tools.miniterm /dev/ttyUSB0 115200
```

### 8.4 React to the user button (PA0, `sw0`)

```c
#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

gpio_pin_configure_dt(&button, GPIO_INPUT);
/* in the loop: */
bool pressed = gpio_pin_get_dt(&button) > 0;
```

---

## 9. Alternative: flash via SWD (ST-Link/J-Link)

If you have a debug probe, solder a 4-pin header (3V3, GND, SWCLK=PA14, SWDIO=PA13)
and flash without touching BOOT0:

```bash
west flash --runner openocd       # with ST-Link
west flash --runner jlink         # with J-Link
west debug                        # attach a GDB session
```

---

## 10. Troubleshooting

| Symptom | Fix |
|---|---|
| `west: command not found` | `source ~/zephyrproject/.venv/bin/activate` |
| `west build` says board not found | Check the name: `west boards \| grep -i blackpill`. It must be `blackpill_f401cc` for the F401CCU6 chip. |
| `Board does not define a usable 'led0' alias` build error | You're targeting a board without a `led0` alias — double-check `-b blackpill_f401cc`. |
| `dfu-util: No DFU capable USB device found` | Re-do the BOOT0+NRST dance (Section 5, Step 2); verify with `lsusb \| grep -i dfu`. |
| `west flash` fails with old dfu-util | Ubuntu's package may be < 0.8 — build dfu-util from source. |
| Flash works but LED doesn't blink | Press NRST after flashing. Check you built for `blackpill_f401cc` (not `f401ce`/`f411ce`). |
| `Permission denied` on USB | Fix udev rules: https://docs.zephyrproject.org/latest/develop/beyond-GSG.html#setting-udev-rules then `sudo udevadm control --reload-rules` and replug. |
| Serial console shows nothing | USB-C is not UART — you must wire PA9/PA10 to a USB-TTL dongle, 115200 8N1. |
| Weird build behavior after config edits | `west build -p always -b blackpill_f401cc .` (pristine). |

---

## 11. Cheat sheet

```bash
# one-time per terminal
source ~/zephyrproject/.venv/bin/activate

# create
mkdir led_toggle && cd led_toggle && mkdir src
# ... write CMakeLists.txt, prj.conf, src/main.c ...

# build
west build -b blackpill_f401cc .
west build                        # incremental rebuild
west build -p always              # pristine rebuild

# flash (DFU: hold BOOT0 -> tap NRST -> release BOOT0)
west flash

# serial console (optional, via USB-TTL on PA9/PA10)
python -m serial.tools.miniterm /dev/ttyUSB0 115200
```

---

*Zephyr v4.4.0-rc1 flow — Ubuntu host, SDK 1.0.0.*
