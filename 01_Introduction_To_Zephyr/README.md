

# Zephyr RTOS — The Essential Guide

> A complete, hands-on introduction to the Zephyr Real-Time Operating System:
> from the ecosystem and architecture, to building and flashing your first
> application on real hardware.

---


## 1. Introduction

**Zephyr** is a modern, small, and scalable **Real-Time Operating System (RTOS)**
designed for real embedded products — not just demos or academic projects.

---
## 2. A Brief History of Zephyr

Zephyr did not appear all at once — it evolved from earlier embedded OS projects.

### Before Zephyr

- Early embedded OS projects such as **Virtuoso** and **ROCKOS** existed and
  later evolved into what we have today.
- These projects contributed ideas, code, and experience that shaped Zephyr's design.

### The Milestone: 2016

> **In 2016, the Zephyr Project was officially launched under the Linux Foundation.**

Since then, Zephyr has grown into a **stable, open, and widely adopted RTOS**,
supporting many architectures and shipping in real products.

---

## 3. What Is Zephyr?

Zephyr is a **modern, small, and scalable RTOS** built for **real embedded products**.

### Key characteristics

| Characteristic | Description |
|----------------|-------------|
| **Modular** | You only enable what your application really needs. This keeps firmware small, organized, and easier to maintain |
| **Portable** | The same application can run on many different boards because hardware details are abstracted. Move from one vendor to another with minimal changes |
| **Open** | Maintained by the Linux Foundation, with long-term support and contributions from many companies and developers |

All of these points together make Zephyr a **strong choice for professional embedded development**.

---

## 4. Key Features

### 4.1 Modular Architecture

Zephyr is built on a modular architecture. In practice this means:

- You **don't get a fixed kernel with everything enabled**.
- If your application only needs GPIOs and timers, you **don't compile** the
  network stack, file system, or USB support.

```
Example:
  Application needs:  GPIO + Timer
  Kernel includes:    GPIO driver, timer subsystem
  Kernel excludes:    Network stack, USB, File System  (not compiled)
```

### 4.2 Powerful Configuration System

Zephyr uses a configuration system based on:

- **Kconfig** — selects which features, drivers, and subsystems are enabled
- **CMake** — drives the build process
- **West** — the meta-tool that coordinates everything

This combination gives you **fine control** over what goes into your firmware
**without manually editing Makefiles or driver code**.

### 4.3 Connectivity

Zephyr supports a wide range of connectivity options, designed to work across
different platforms:

- **Bluetooth** (Classic & Low Energy)
- **Networking** (TCP/IP, MQTT, CoAP, LwM2M ...)
- **USB**
- **Serial / UART**
- **CAN, LoRaWAN, 802.15.4, Wi-Fi ...**

### 4.4 Hardware Abstraction via Device Tree

Instead of hardcoding hardware details in the application, hardware is
described using **Device Tree** files (`.dts` / `.overlay`).

This enables **portability across boards and vendors without changing application code**.

### 4.5 Security & Boot

Zephyr integrates MCUboot, enabling:

- **Secure Boot**
- **Firmware Update (FWU / DFU) workflows**

### 4.6 Safety & Security Mechanisms

Zephyr includes:

- **Memory Protection** (MPU/MMU-based)
- **User Mode** execution
- **Stack Overflow Detection**
- **Other safety mechanisms**

### 4.7 License

Zephyr is released under the **Apache License 2.0**, which is permissive and
suitable for both **open-source and commercial products**.

### 4.8 Long-Term Support (LTS)

Zephyr is product-development ready and offers **LTS (Long-Term Support)
releases** that provide stability and maintenance over time.

---

## 5. Ecosystem & Community

### Industry backing

A lot of major companies contribute to Zephyr, including:

- **Intel**, **Nordic Semiconductor**, **NXP**, **STMicroelectronics (ST)**,
  **Google**, **Meta** ...and many others

### Global community

In addition to corporate contributors, Zephyr has a **large global developer
community** with **thousands of developers collaborating worldwide**.

---

## 6. Supported Architectures

Zephyr supports multiple CPU architectures:

| Architecture | Typical Use |
|--------------|-------------|
| **ARM (ARMv6/7/8-M, Cortex-A/R)** | Microcontrollers & embedded processors |
| **RISC-V** | Open ISA, modern MCUs |
| **x86** | Intel-based embedded |
| **Xtensa** | Audio/DSP processors (e.g., Espressif) |
| **ARC** | Synopsys processors |

---

## 7. System Architecture

Zephyr is **layered**:

```
┌─────────────────────────────────────────────────────────┐
│                 APPLICATION SERVICES                    │
│   (your applications — portable, standardized API)      │
├─────────────────────────────────────────────────────────┤
│              OS SERVICES & HIGH-LEVEL APIs              │
│   Logging, File Systems, IPC, Networking, Device Mgmt   │
├─────────────────────────────────────────────────────────┤
│                    KERNEL + LOW-LEVEL DRIVERS           │
│   Scheduling, Memory, Power Management, Hardware access │
├─────────────────────────────────────────────────────────┤
│                      HARDWARE                           │
│              (Boards, SoCs, Peripherals)                │
└─────────────────────────────────────────────────────────┘
```

### Layer-by-layer

1. **Kernel & Low-Level Drivers** — scheduling, memory, power management,
   direct hardware access. Examples: kernel drivers, OS services, low-level APIs.
2. **OS Services & High-Level APIs** — logging, file systems, IPC, networking,
   device management.
3. **Application Services** — your applications interact with the system in a
   **portable and standardized way**, independent of hardware.
4. **Community Ecosystem** — community libraries and integrations **extend the
   ecosystem without modifying the OS core**.

---

## 8. Anatomy of a Zephyr Application

A typical Zephyr application is organized into a few main files. Each file has a different responsibility: **build configuration, hardware description, and application logic**.

```text
my_app/
├── CMakeLists.txt          ← tells the build system what to compile
├── prj.conf                ← base/common Kconfig configuration
├── debug.conf              ← optional debug-specific Kconfig settings
├── release.conf            ← optional release-specific Kconfig settings
├── src/
│   └── main.c              ← the application logic
└── app.overlay             ← optional Device Tree modifications
```

The important idea is:

```text
                    Zephyr Application
                           │
             ┌─────────────┼─────────────┐
             │             │             │
          Kconfig       DeviceTree      Code
             │             │             │
         *.conf        *.overlay       *.c / *.h
             │             │             │
      Build features   Hardware       Application
      & options        description     behavior
```

---

### 8.1 `prj.conf` — Base Kconfig Configuration

`prj.conf` contains the **common/default configuration** for the application that updates the kconfig default values.

For example:

```ini
CONFIG_CONSOLE=y
CONFIG_LOG=y
CONFIG_GPIO=y
CONFIG_BLUETOOTH=y
```

These options enable Zephyr features, subsystems, drivers, and other build-time configurations.

You can enable logging, Bluetooth, GPIO, or a specific driver **without changing the application source code**.

`prj.conf` is normally the **base configuration** from which the application starts.

---

### 8.2 Additional `.conf` Files — Build Variants

You can have additional Kconfig configuration files for different build variants.

For example:

```text
my_app/
├── prj.conf
├── debug.conf
└── release.conf
```

`debug.conf` might contain:

```ini
CONFIG_DEBUG_OPTIMIZATIONS=y
CONFIG_ASSERT=y
CONFIG_LOG_DEFAULT_LEVEL=4
```

while `release.conf` might contain:

```ini
CONFIG_SIZE_OPTIMIZATIONS=y
CONFIG_ASSERT=n
CONFIG_LOG_DEFAULT_LEVEL=1
```

The additional file can **add new configuration options or override values from `prj.conf`**.

You select the desired configuration when building:

```bash
# Debug build
west build -b my_board . -- -DOVERLAY_CONFIG=debug.conf
```

or:

```bash
# Release build
west build -b my_board . -- -DOVERLAY_CONFIG=release.conf
```

Conceptually:

```text
                 prj.conf
              Base configuration
                     │
          ┌──────────┴──────────┐
          │                     │
     debug.conf             release.conf
          │                     │
          ▼                     ▼
    Debug build           Release build
```

So you can maintain **one common `prj.conf`** and customize it for different build configurations instead of duplicating the entire configuration.

> **Important:** `debug.conf` and `release.conf` are Kconfig configuration fragments. They are different from Device Tree `.overlay` files, even though both mechanisms use the word "overlay" in different contexts.

---

### 8.3 `CMakeLists.txt` — Build Definition

`CMakeLists.txt` tells Zephyr's build system how the application is structured and which source files should be compiled.

```cmake
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(my_app)

target_sources(app PRIVATE
    src/main.c
)
```

In modern Zephyr development, you normally interact with this through **CMake and `west`**, rather than directly managing a traditional Makefile.

For example:

```bash
west build -b my_board
```

The build process roughly becomes:

```text
west
  ↓
CMake
  ↓
Zephyr Build System
  ↓
Kconfig + DeviceTree + Application Source
  ↓
Compiler / Linker
  ↓
Firmware
```

---

### 8.4 `main.c` — Application Logic

`main.c` contains the actual application behavior.

For example:

```c
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

int main(void)
{
    while (1) {
        /* Application logic */
        k_msleep(1000);
    }

    return 0;
}
```

This is where you implement things such as:

* Application logic
* Threads
* Workqueues
* Driver interactions
* Communication
* State machines
* Sensor processing
* Application behavior

The application code uses the configuration and hardware information provided by **Kconfig and DeviceTree**.

---

### 8.5 `app.overlay` — Device Tree Overlay

The `.overlay` file is used to **modify or extend the board's Device Tree**.

For example:

```dts
/ {
    leds {
        compatible = "gpio-leds";

        led0: led_0 {
            gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;
        };
    };
};
```

It can be used to:

* Enable a peripheral
* Disable a peripheral
* Change GPIO pins
* Configure buses
* Add devices
* Describe hardware that is not present in the default board Device Tree

For example, the board might already have a UART node, but you can use an overlay to modify its configuration.

---

### 8.6 Multiple Device Tree Overlays

You are **not limited to one `.overlay` file**.

You could have:

```text
my_app/
├── app.overlay
└── boards/
    ├── board1.overlay
    └── board2.overlay
```

Different overlays can be selected depending on the board or explicitly through the build configuration.

For example:

```bash
west build -b board1
```

can use the appropriate board-specific Device Tree configuration.

You can also explicitly specify an overlay:

```bash
west build -b my_board . -- \
    -DDTC_OVERLAY_FILE=board1.overlay
```

---

### 8.7 Putting Everything Together

The complete relationship looks like this:

```text
                         Zephyr Application
                                │
        ┌───────────────────────┼───────────────────────┐
        │                       │                       │
     Kconfig                DeviceTree              Source Code
        │                       │                       │
   ┌────┴────┐             ┌────┴────┐                  │
   │         │             │         │                  │
prj.conf  debug.conf    app.overlay board.overlay     main.c
   │      release.conf     │         │                  │
   └────┬────┘             └────┬────┘                  │
        │                       │                       │
        ▼                       ▼                       ▼
    Build options      Hardware description        Application
          │                     │                    behavior
          └─────────────────────┼───────────────────────┘
                                │
                          CMake / West
                                │
                                ▼
                           Zephyr Build
                                │
                                ▼
                            Firmware
```

### The key distinction

| File             | Purpose                             |        Can have multiple? | How selected                      |
| ---------------- | ----------------------------------- | ------------------------: | --------------------------------- |
| `prj.conf`       | Base Kconfig configuration          |               Usually one | Automatically                     |
| `debug.conf`     | Debug Kconfig additions/overrides   |                       Yes | `OVERLAY_CONFIG`                  |
| `release.conf`   | Release Kconfig additions/overrides |                       Yes | `OVERLAY_CONFIG`                  |
| `app.overlay`    | Device Tree modifications           |                       Yes | Build system / explicit selection |
| `board.overlay`  | Board-specific Device Tree          |                       Yes | Based on selected board / build   |
| `CMakeLists.txt` | Build definition                    |       Usually one per app | CMake                             |
| `main.c`         | Application logic                   | Many `.c` files can exist | CMake                             |

> **In short:** `prj.conf` provides the common Kconfig settings, additional `.conf` files can modify or extend them for different build variants, `.overlay` files describe hardware through DeviceTree, `CMakeLists.txt` defines the build, and `main.c` contains the application logic.

---

## 9. The Build System Deep Dive

**West** coordinates the entire workflow with a single command. It manages
project configuration, triggers the build, and flashes the firmware.

### Behind the scenes

| Tool | Role |
|------|------|
| **CMake** | Configures the build — selects toolchain, board, application, and all required modules |
| **Kconfig** | Determines which kernel features, drivers, and subsystems are enabled |
| **Devicetree** | Merges board definition + app overlay, generates C headers automatically |
| **Ninja** | Compiles & links kernel, drivers, libraries, and the app → final firmware image |
| **west flash** | Entry point for downloading and flashing to your board |

---

## 10. Getting Started: Installation

> Exact steps vary by OS (Linux, Ubuntu, macOS, Windows).

```bash
# 1. Install West
pip install west

# 2. Get the Zephyr source code
west init zephyrproject
cd zephyrproject
west update

# 3. Install Python dependencies
pip install -r zephyr/scripts/requirements.txt
```

> The **Zephyr SDK** contains the toolchains, compilers, and linkers needed to
> build Zephyr firmware — install it alongside West.

---

## 11. Hands-On Demo: Blinky LED

### 11.1 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20.0)

# Locate and load the Zephyr build system
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Define the project name
project(blinky)

# Add the application source file
target_sources(app PRIVATE src/main.c)
```

| Line | Purpose |
|------|---------|
| `cmake_minimum_required(...)` | Sets the minimal required CMake version |
| `find_package(Zephyr ...)` | Locates and loads the Zephyr build system — integrates kernel, drivers, configuration and hardware |
| `project(blinky)` | Defines the project name |
| `target_sources(app PRIVATE src/main.c)` | Adds `main.c` to the application sources |

### 11.2 main.c

```c
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS   1000
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
    int ret;

    /* Check if the GPIO device is ready */
    if (!gpio_is_ready_dt(&led)) {
        return;
    }

    /* Configure the LED pin as output */
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return;
    }

    while (1) {
        /* Toggle the LED */
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            return;
        }
        k_msleep(SLEEP_TIME_MS);
    }
}
```

**Line-by-line:**

- `#include <zephyr/kernel.h>` — imports the Zephyr kernel (for `k_msleep`, loops)
- `#include <zephyr/drivers/gpio.h>` — imports the GPIO driver
- `#define SLEEP_TIME_MS 1000` — sets the 1-second wait time
- `DT_ALIAS(led0)` — gets the LED from the **Device Tree** → **portability between boards**
- `GPIO_DT_SPEC_GET(...)` — creates a structure with the pin and flags for the LED
- `gpio_is_ready_dt(&led)` — checks if the GPIO device is ready to use
- `gpio_pin_configure_dt(...)` — sets the LED as output and enables it
- `while (1) { gpio_pin_toggle_dt(...); }` — main loop: toggles the LED

### 11.3 Build

```bash
# Clean any previous build
west build -p always

# Build for a specific board
west build -p always -b nrf52840dk_nrf52840
```

- `-p always` → pristine (clean) build every time
- `-b nrf52840dk_nrf52840` → target board (e.g., a Nordic development kit)
- If your board has multiple SoCs/cores, specify the core as needed

### 11.4 Output

```
build/
└── zephyr/
    ├── zephyr.hex    ← firmware image (Intel HEX)
    ├── zephyr.bin    ← firmware image (raw binary)
    └── zephyr.elf    ← ELF with debug symbols
```

### 11.5 Flash

```bash
# Many boards (e.g., Nordic nRF52840 DK) have an integrated J-Link probe
west flash

# Or specify a runner explicitly
west flash --runner pyocd
```

Once flashed, the **LED starts blinking** on your board! 🎉

---

## 12. Enabling Logging & Console Output

To **print the LED status in a terminal**, configure `prj.conf`:

```ini
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
```

Add logging to `main.c`:

```c
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(blinky, LOG_LEVEL_DBG);

void main(void)
{
    LOG_INF("Blinky started!");
    while (1) {
        gpio_pin_toggle_dt(&led);
        LOG_INF("LED state: %s",
                gpio_pin_get_dt(&led) ? "ON" : "OFF");
        k_msleep(SLEEP_TIME_MS);
    }
}
```

Rebuild, reflash, and open a serial terminal (e.g., `minicom -D /dev/ttyACM1 -b 115200`)
to see LED status live, synchronized with the blinking LED on the board.

---

## 13. Zephyr in Real Products

| Strength | Why it matters |
|----------|----------------|
| **Modularity** | Small firmware, organized, easier to maintain |
| **Portability** | Same app runs on many boards; vendor changes are minimal |
| **Connectivity** | Bluetooth, network, USB — works across platforms |
| **Security** | MCUboot, secure boot, firmware updates |
| **Safety mechanisms** | Memory protection, user mode, stack overflow detection |
| **Apache 2.0 license** | Permissive — fine for open and commercial products |
| **LTS releases** | Stability and maintenance over time |

---

## 14. Common Commands Cheat Sheet

| Command | Description |
|---------|-------------|
| `west init zephyrproject` | Initialize a new Zephyr workspace |
| `west update` | Fetch/update all project modules |
| `west build` | Build the application (incremental) |
| `west build -p always` | Pristine (clean) build |
| `west build -b <board>` | Build for a specific board |
| `west flash` | Flash firmware to the board |
| `west flash --runner pyocd` | Flash using a specific runner |
| `west build -t menuconfig` | Open the interactive Kconfig menu |
| `west build -t guiconfig` | Open the graphical Kconfig editor |
| `west boards` | List all supported boards |

---

## 15. Troubleshooting

| Problem | Solution |
|---------|----------|
| `cmake` can't find Zephyr | Make sure `ZEPHYR_BASE` is set, or use `west build` from the workspace root |
| Board not found | Run `west boards`; check board name spelling |
| Flash fails | Check USB connection, drivers, and that the debug probe (J-Link) is detected |
| Nothing on serial terminal | Verify `CONFIG_CONSOLE=y` and `CONFIG_UART_CONSOLE=y`; check correct serial port & baud rate |
| Build errors after config change | Use a pristine build: `west build -p always` |
| LED not blinking | Check the Device Tree alias (`led0`) and GPIO definition in the overlay |

---

## 16. Glossary

| Term | Meaning |
|------|---------|
| **RTOS** | Real-Time Operating System |
| **Zephyr** | The open-source RTOS maintained by the Linux Foundation |
| **West** | Zephyr's meta-tool: project management, build, flash |
| **Kconfig** | Configuration system — selects features/drivers at compile time |
| **Devicetree** | Hardware description format — abstracts board/SoC details from code |
| **Overlay** | A `.overlay` file that extends/modifies the board's Device Tree |
| **CMake** | Build system generator used by Zephyr |
| **Ninja** | Low-level build tool that compiles and links everything |
| **MCUboot** | Secure bootloader — secure boot & firmware updates |
| **LTS** | Long-Term Support release |
| **HAL** | Hardware Abstraction Layer |
| **GPIO** | General Purpose Input/Output |
| **IPC** | Inter-Process Communication |
| **DFU** | Device Firmware Update |
| **J-Link** | Common debug probe / programmer for ARM boards |

---

## 17. License

- **Zephyr RTOS**: Apache License 2.0 — permissive, suitable for both
  open-source and commercial products.

---



