# Zephyr RTOS — A Simple Introduction

This guide explains what Zephyr is, why people use it, and how a Zephyr
application is put together — in plain language.

---

## 1. What is an RTOS?

An **RTOS** (Real-Time Operating System) is a small operating system for
microcontrollers.

Normal software runs "as fast as it can". An RTOS runs tasks with **predictable
timing** — it can guarantee that important work happens within a certain time.
That is why it is used in cars, medical devices, drones, and IoT products.

---

## 2. What is Zephyr?

**Zephyr** is a free, open-source RTOS made for real embedded products.

Think of it as a toolbox:

- It gives you a **kernel** (tasks, scheduling, timers, memory).
- It gives you **drivers** for hardware (GPIO, UART, I2C, SPI, USB...).
- It gives you **services** (Bluetooth, networking, file systems, logging).
- You only turn on the parts you need, so the firmware stays small.

Zephyr started in **2016** under the **Linux Foundation**. It grew from earlier
embedded OS projects (Virtuoso, ROCKOS) and is now used in real commercial
products.

---

## 3. Why use Zephyr?

| Reason | Simple meaning |
|---|---|
| **Modular** | Only compile what you need — smaller firmware |
| **Portable** | Same app runs on many boards with little change |
| **Open** | Free to use, Apache 2.0 license, backed by many companies |
| **Connected** | Bluetooth, Wi-Fi, USB, networking built in |
| **Safe** | Memory protection, secure boot, firmware updates |
| **Supported** | Long-Term Support (LTS) releases for products |

---

## 4. Which CPUs does it support?

Zephyr runs on most embedded CPUs:

- **ARM Cortex-M / A / R** (STM32, Nordic, NXP...)
- **RISC-V**
- **x86**
- **Xtensa** (ESP32)
- **ARC**

You write code once, and Zephyr handles the differences between boards.

---

## 5. How Zephyr is organized

Zephyr is built in layers. Each layer only talks to the layer below it:

```
┌─────────────────────────────────────┐
│ Your application                    │  ← your code
├─────────────────────────────────────┤
│ Zephyr services                     │  ← logging, networking, Bluetooth
├─────────────────────────────────────┤
│ Kernel + drivers                    │  ← scheduling, GPIO, UART, I2C
├─────────────────────────────────────┤
│ Hardware                            │  ← the actual board and chip
└─────────────────────────────────────┘
```

**Important rule:** your code uses Zephyr APIs. It never touches the hardware
registers directly. This is what makes the same code work on many boards.

---

## 6. What is inside a Zephyr application?

A Zephyr app is just a folder with a few files:

```
my_app/
├── CMakeLists.txt     ← tells the build system what to compile
├── prj.conf           ← turns features on/off (Kconfig)
├── src/main.c         ← your application code
└── app.overlay        ← optional: describes hardware changes (Devicetree)
```

Each file has one job:

| File | Job | Language |
|---|---|---|
| `CMakeLists.txt` | Build definition — which source files to compile | CMake |
| `prj.conf` | Feature switches — enable GPIO, logging, Bluetooth... | Kconfig |
| `src/main.c` | Application logic — what your program does | C |
| `app.overlay` | Hardware description — which pins, buses, devices | Devicetree |

The big idea:

- **Kconfig** decides **what software features** are included.
- **Devicetree** decides **what hardware exists** and which pins are used.
- **C code** uses both through portable APIs.

---

## 7. How the build works

You usually only type one command:

```bash
west build -b <board_name> .
```

Behind the scenes:

```
west  →  CMake  →  Kconfig + Devicetree  →  Ninja  →  firmware
```

- **west** — Zephyr's tool for managing the workspace, building, and flashing
- **CMake** — sets up the build
- **Kconfig** — picks software features
- **Devicetree** — picks hardware configuration
- **Ninja** — compiles and links everything

The result is a firmware file you can flash to the board.

---

## 8. First example: blink an LED

The classic first program toggles an LED once per second.

**`CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky)

target_sources(app PRIVATE src/main.c)
```

**`src/main.c`**

```c
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

    while (1) {
        gpio_pin_toggle_dt(&led);
        k_msleep(1000);
    }
}
```

**What the code does, simply:**

1. `DT_ALIAS(led0)` asks the Devicetree: "which pin is the LED?"
2. `gpio_pin_configure_dt()` sets that pin as an output.
3. `gpio_pin_toggle_dt()` flips it on/off.
4. `k_msleep(1000)` waits one second.
5. The loop repeats forever.

**Build and flash:**

```bash
west build -b nrf52840dk_nrf52840 .
west flash
```

The LED should start blinking.

---

## 9. Useful commands

| Command | What it does |
|---|---|
| `west init` / `west update` | Download Zephyr and its modules |
| `west boards` | List supported boards |
| `west build -b <board>` | Build the app for a board |
| `west build -p always -b <board>` | Clean build from scratch |
| `west flash` | Flash the board |
| `west build -t menuconfig` | Open the Kconfig menu |
| `west debug` | Start a debugger session |

---

## 10. Simple glossary

| Term | Meaning |
|---|---|
| **RTOS** | Real-Time Operating System |
| **Kernel** | The core that runs tasks and manages time/memory |
| **West** | Zephyr's command-line tool |
| **Kconfig** | System that switches software features on/off |
| **Devicetree** | Files that describe the hardware |
| **Overlay** | A Devicetree file that changes the board's default hardware setup |
| **CMake / Ninja** | Tools that build the firmware |
| **MCUboot** | Secure bootloader for firmware updates |
| **HAL** | Hardware Abstraction Layer — vendor low-level code |

---

## 11. Common problems

| Problem | Fix |
|---|---|
| `west: command not found` | Activate the Python venv first |
| Board not found | Run `west boards` and check the name |
| Flash fails | Check USB cable, drivers, and board boot mode |
| Build errors after config change | Do a clean build: `west build -p always` |
| LED not blinking | Check the board's `led0` alias in its Devicetree |

---

## 12. Summary

- Zephyr is a small, open RTOS for embedded products.
- You enable only the features you need.
- Kconfig = software features; Devicetree = hardware description.
- `west build` builds; `west flash` flashes.
- The same app can run on many boards because hardware is not hard-coded.

Next: install Zephyr in `02_Installation_Guide_What_Gets_Installed`, then create
your first project in `03_ Create_New_Project`.
