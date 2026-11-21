# Create Your First Zephyr Project — Simple Guide

**Goal:** create a new Zephyr app that blinks the on-board LED on the
**WeAct Black Pill V1.2 (STM32F401CC)**.

**Board target:** `blackpill_f401cc`

**Before you start:** Zephyr must already be installed. In every new terminal:

```bash
source ~/zephyrproject/.venv/bin/activate
```

---

## 1. Know your board

Check that Zephyr knows the board:

```bash
west boards | grep -i blackpill
```

Important facts about the Black Pill F401CC:

| Item | Value |
|---|---|
| Board name | `blackpill_f401cc` |
| On-board LED | PC13, **active-low** (alias `led0`) |
| User button | PA0 (alias `sw0`) |
| Console UART | PA9 (TX), PA10 (RX), 115200 |
| CPU | STM32F401CCU6, Cortex-M4F, 84 MHz |
| Flash | 256 KB |
| SRAM | 64 KB |
| Flashing | USB DFU bootloader (no ST-Link needed) |

> The USB-C port is **power + DFU only**, not a serial port. To see `printk`
> output, you need a USB-TTL adapter on PA9/PA10.

---

## 2. Create the project folder

A Zephyr app needs only three files: `CMakeLists.txt`, `prj.conf`, and
`src/main.c`.

```bash
cd ~/zephyrproject
mkdir led_toggle && cd led_toggle
mkdir src
```

Your folder will look like this:

```
led_toggle/
├── CMakeLists.txt     ← build instructions
├── prj.conf           ← feature switches
└── src/
    └── main.c         ← application code
```

The Black Pill already defines `led0` = PC13, so no extra Devicetree file is
needed for a simple blink.

---

## 3. Write `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(led_toggle)

target_sources(app PRIVATE src/main.c)
```

What each line means:

| Line | Meaning |
|---|---|
| `cmake_minimum_required(...)` | Minimum CMake version needed |
| `find_package(Zephyr ...)` | Finds and loads Zephyr |
| `project(led_toggle)` | Sets the app name |
| `target_sources(app PRIVATE src/main.c)` | Tells the build to compile `main.c` |

---

## 4. Write `prj.conf`

```conf
CONFIG_GPIO=y
```

This file uses **Kconfig**. Each `CONFIG_*` line turns a feature on or off at
compile time.

`CONFIG_GPIO=y` means: "include the GPIO driver". Without it, the GPIO functions
in `main.c` would not link.

You can add more features later, for example:

```conf
CONFIG_GPIO=y
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
```

---

## 5. Write `src/main.c`

```c
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Ask the Devicetree which pin is the LED. */
#define LED0_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Board does not define a usable 'led0' alias"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
    int ret;

    /* Make sure the GPIO driver is ready. */
    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    /* Configure the LED pin as output, initially off. */
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        return 0;
    }

    while (1) {
        /* Flip the LED on/off. */
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return 0;
        }

        /* Wait 500 ms. Full blink period = 1 second. */
        k_msleep(500);
    }

    return 0;
}
```

---

## 6. Build

From the project folder:

```bash
west build -b blackpill_f401cc .
```

That is all. West and CMake will:

1. Read the board's Devicetree and Kconfig.
2. Compile Zephyr and your app with the ARM toolchain.
3. Create the firmware in `build/zephyr/`.

Useful build commands:

```bash
west build -b blackpill_f401cc .    # first build
west build                          # rebuild after edits
west build -p always -b blackpill_f401cc .   # clean build from scratch
```

---

## 7. Flash the board (USB DFU — no ST-Link needed)

The STM32F401 has a built-in USB bootloader, so you only need a USB-C cable.

### Step 1 — install dfu-util

```bash
sudo apt install dfu-util
dfu-util --version
```

### Step 2 — enter DFU mode

1. Plug the board into USB-C.
2. **Hold BOOT0**.
3. Press and release **NRST**.
4. Release **BOOT0**.

Check that the board is in DFU mode:

```bash
lsusb | grep -i dfu
# Should show: 0483:df11 STM Device in DFU Mode
```

### Step 3 — flash

```bash
west flash
```

This runs roughly:

```bash
dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D build/zephyr/zephyr.bin
```

### Step 4 — run

Press **NRST** once. Your app starts.

---

## 8. What you should see

The on-board LED blinks:

- **ON for 500 ms**
- **OFF for 500 ms**

That is a 1 Hz blink.

The LED is **active-low**. Zephyr handles the polarity automatically because the
board's Devicetree says `GPIO_ACTIVE_LOW`. You just call
`gpio_pin_toggle_dt()`.

---

## 9. How the code works (simple version)

| Code | What it does |
|---|---|
| `#include <zephyr/kernel.h>` | Gives you `k_msleep()` and other kernel functions |
| `#include <zephyr/drivers/gpio.h>` | Gives you GPIO functions |
| `DT_ALIAS(led0)` | Finds the LED node in the Devicetree |
| `DT_NODE_HAS_STATUS(..., okay)` | Stops the build with a clear error if there is no LED |
| `GPIO_DT_SPEC_GET(...)` | Builds a struct with the GPIO port, pin, and flags |
| `gpio_is_ready_dt()` | Checks the GPIO driver is ready |
| `gpio_pin_configure_dt()` | Sets the pin as an output |
| `gpio_pin_toggle_dt()` | Flips the pin |
| `k_msleep(500)` | Waits 500 ms without wasting CPU |

**Why use Devicetree macros instead of writing "PC13"?**
Because the same code can run on another board where `led0` is a different pin.
Only the Devicetree changes — the C code stays the same.

---

## 10. Customize

### 10.1 Change the blink speed

```c
k_msleep(100);   /* 100 ms on, 100 ms off = fast blink */
```

### 10.2 Use a different pin with an overlay

If you want the LED on PC14 instead of PC13, create a Devicetree overlay.
Make a `boards/` folder and add a file named after the board:

```
led_toggle/boards/blackpill_f401cc.overlay
```

Content:

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

Rebuild, and `DT_ALIAS(led0)` now points to PC14. You do not change `main.c`.

Rules for overlays:

- `boards/<board_name>.overlay` is used only for that board.
- `app.overlay` in the app root is used for all boards.
- No overlay means the board's default LED (PC13) is used.

### 10.3 Add log output over UART

`prj.conf`:

```conf
CONFIG_GPIO=y
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
```

`main.c`:

```c
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_toggle, LOG_LEVEL_INF);

/* inside main(), before the loop: */
LOG_INF("LED toggle app starting");
```

Connect PA9 to a USB-TTL RX, PA10 to TX, and GND to GND (3.3 V only), then:

```bash
pip install pyserial
python -m serial.tools.miniterm /dev/ttyUSB0 115200
```

### 10.4 Read the user button (PA0, `sw0`)

```c
#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

gpio_pin_configure_dt(&button, GPIO_INPUT);
bool pressed = gpio_pin_get_dt(&button) > 0;
```

---

## 11. Alternative: flash with ST-Link or J-Link

If you have an SWD debug probe, connect 3V3, GND, SWCLK, and SWDIO, then:

```bash
west flash --runner openocd    # ST-Link
west flash --runner jlink      # J-Link
west debug                     # start a debug session
```

---

## 12. Common problems

| Problem | Fix |
|---|---|
| `west: command not found` | Activate the venv: `source ~/zephyrproject/.venv/bin/activate` |
| Board not found | Run `west boards \| grep -i blackpill`; the name must be exact |
| `led0` alias error | You built for the wrong board — use `-b blackpill_f401cc` |
| `No DFU capable USB device found` | Repeat the BOOT0 + NRST steps |
| `west flash` fails with old dfu-util | Install a newer `dfu-util` |
| LED does not blink | Press NRST after flashing; confirm the board name |
| Permission denied on USB | Fix udev rules and replug the board |
| No serial output | USB-C is not UART — use a USB-TTL adapter on PA9/PA10 |
| Strange build errors | Do a clean build: `west build -p always` |

---

## 13. Cheat sheet

```bash
# every new terminal
source ~/zephyrproject/.venv/bin/activate

# create the project
mkdir led_toggle && cd led_toggle
mkdir src
# add CMakeLists.txt, prj.conf, src/main.c

# build
west build -b blackpill_f401cc .
west build -p always -b blackpill_f401cc .   # clean build

# flash (hold BOOT0, tap NRST, release BOOT0 first)
west flash

# optional serial console
python -m serial.tools.miniterm /dev/ttyUSB0 115200
```
