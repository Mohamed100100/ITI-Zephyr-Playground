# Zephyr Installation — Simple Guide

This guide installs Zephyr on **Ubuntu 24.04+** and then explains **what got
installed** on your disk.

**Workspace used below:** `~/zephyrproject`

The installation has two parts:

1. **Install** — run the commands in order.
2. **Understand** — see what each installed folder is for.

---

# PART 1 — INSTALL

## 1. What you are installing

You are not installing one app. You are creating a **West workspace**:

- the Zephyr OS source code
- all external dependencies (vendor drivers, libraries, bootloader)
- the Zephyr SDK (cross-compilers and tools)
- a Python environment for the `west` tool

Everything lives in one folder:

```
~/zephyrproject/
```

---

## 2. Install the required packages

Update the system:

```bash
sudo apt update
sudo apt upgrade
```

Install the tools Zephyr needs:

```bash
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3-dev python3-venv python3-tk \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
```

What the important ones do:

| Package | Why it is needed |
|---|---|
| `git` | Downloads Zephyr and its modules |
| `cmake` / `ninja-build` | Build the firmware |
| `gperf` | Generates lookup tables used by Zephyr |
| `ccache` | Speeds up rebuilds |
| `dfu-util` | Flashes STM32 boards over USB (Black Pill) |
| `device-tree-compiler` | Checks Devicetree files |
| `python3-venv` | Creates an isolated Python environment |
| `libsdl2-dev` | Graphics support for the `native_sim` board |

> On **ARM64**, skip `gcc-multilib` and `g++-multilib` — they don't exist there.

Check the versions:

```bash
cmake --version      # 3.28.0 or newer
python3 --version    # 3.12 recommended
dtc --version        # 1.4.6 or newer
```

---

## 3. Create a Python virtual environment

A virtual environment keeps Zephyr's Python packages separate from your system.

```bash
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
```

Your prompt should now start with `(.venv)`.

> Run this `source` command **every time you open a new terminal**.

---

## 4. Install West

West is Zephyr's main tool. It downloads, builds, flashes, and debugs.

```bash
pip install west
west --version
```

---

## 5. Download Zephyr

```bash
west init -m https://github.com/zephyrproject-rtos/zephyr ~/zephyrproject
cd ~/zephyrproject
west update
```

What happens:

1. `west init` marks the folder as a workspace and downloads the Zephyr source.
2. `west update` reads `zephyr/west.yml` and downloads about 70 external
   projects (vendor drivers, libraries, MCUboot...), each pinned to an exact
   version.

---

## 6. Install Python dependencies and register Zephyr

```bash
west packages pip --install
west zephyr-export
```

- `west packages pip --install` installs the Python packages this Zephyr
  version needs.
- `west zephyr-export` registers Zephyr with CMake, so `find_package(Zephyr)`
  works without setting paths by hand.

---

## 7. Install the Zephyr SDK

The SDK contains the cross-compilers (GCC), QEMU, and OpenOCD for all supported
CPUs.

```bash
cd ~/zephyrproject/zephyr
west sdk install
```

Useful options:

```bash
west sdk install --install-dir ~/my-sdk          # custom location
west sdk install --toolchains arm-zephyr-eabi    # only ARM
```

---

## 8. Test that it works

Open a new terminal, then:

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject
```

### Test A — run in simulation (no hardware)

```bash
west build -p always -b native_sim samples/hello_world
west build -t run
```

Expected output:

```
*** Booting Zephyr OS ... ***
Hello World! native_sim
```

### Test B — real hardware

Find your board:

```bash
west boards | grep -i <part_of_board_name>
```

Build and flash:

```bash
west build -p always -b <board_name> samples/basic/blinky
west flash
```

If flashing fails, install the tool named in the error (OpenOCD, J-Link,
dfu-util, esptool...).

---

## 9. Black Pill (this workspace's board)

Zephyr supports several WeAct Black Pill variants:

| Board name | Chip | Flash / SRAM |
|---|---|---|
| `blackpill_f401cc` | STM32F401CCU6 | 256 KB / 64 KB |
| `blackpill_f401ce` | STM32F401CEU6 | 512 KB / 96 KB |
| `blackpill_f411ce` | STM32F411CEU6 | 512 KB / 128 KB |
| `blackpill_u585ci` | STM32U585CI | 2 MB / 786 KB |
| `blackpill_h523ce` | STM32H523CE | 512 KB / 256 KB |

Find yours:

```bash
west boards | grep -i blackpill
```

Default pins on the F401/F411 boards:

| Function | Pin |
|---|---|
| On-board LED (`led0`) | PC13, active-low |
| User button (`sw0`) | PA0 |
| Console UART | PA9 (TX), PA10 (RX), 115200 |
| ADC | PA1 |

Build an example:

```bash
west build -p always -b blackpill_f401cc samples/basic/blinky
```

Flash using the built-in DFU bootloader (no ST-Link needed):

1. Install `dfu-util`:
   ```bash
   sudo apt install dfu-util
   ```
2. Enter DFU mode: **hold BOOT0, tap NRST, release BOOT0**.
3. Check the board is detected:
   ```bash
   lsusb | grep -i dfu
   ```
4. Flash:
   ```bash
   west flash
   ```
5. Press **NRST** to start the app.

> The USB-C port is **power + DFU only**, not a serial port. To see text output,
> connect PA9/PA10 to a USB-TTL adapter.

---

## 10. How to update later

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject
west update
west packages pip --install
west zephyr-export
```

---

# PART 2 — WHAT GOT INSTALLED

## 11. The workspace folder

```
zephyrproject/
├── zephyr/       ← the Zephyr OS source (kernel, drivers, boards, samples)
├── modules/      ← external projects: vendor drivers, libraries, filesystems
├── bootloader/   ← MCUboot secure bootloader
├── tools/        ← test and network helpers
├── .west/        ← marks this folder as a West workspace
├── .venv/        ← Python virtual environment
└── build/        ← build output (created after the first build)
```

---

## 12. The `zephyr/` folder

This is the operating system itself:

| Folder | What is inside |
|---|---|
| `arch/` | CPU-specific startup and context switching code |
| `soc/` | Chip-family support (STM32, Nordic, ESP32...) |
| `boards/` | Board definitions and Devicetree files |
| `drivers/` | Zephyr drivers for GPIO, UART, I2C, SPI... |
| `kernel/` | Scheduler, threads, timers, queues, mutexes |
| `subsys/` | Bluetooth, networking, USB, file systems, logging |
| `include/zephyr/` | Public C headers you include in your app |
| `samples/` | Example applications |
| `tests/` | Automated test suite |
| `dts/` | Devicetree bindings and includes |
| `west.yml` | List of external projects and their versions |

---

## 13. The `modules/` folder

These are external projects downloaded by `west update`. You normally do not
edit them.

| Folder | Contents |
|---|---|
| `hal/` | Vendor hardware libraries (ST, Nordic, Espressif, NXP...) |
| `lib/` | Third-party libraries (LVGL, nanopb, picolibc...) |
| `fs/` | File systems (FAT, LittleFS) |
| `crypto/` | Security libraries (mbedTLS...) |
| `debug/` | Debugging tools |
| `tee/` | Trusted firmware (TF-M) |

---

## 14. The SDK

Installed separately by `west sdk install`, it contains:

| Tool | Purpose |
|---|---|
| `arm-zephyr-eabi-gcc` and other compilers | Build for ARM, RISC-V, x86... |
| QEMU | Run boards without hardware |
| OpenOCD | Flash and debug real boards |
| Host tools | Helper programs for builds and flashing |

---

## 15. How a call flows through the layers

When your app toggles an LED:

```
Your code      main.c
  ↓
Zephyr API     gpio_pin_toggle()
  ↓
Zephyr driver  drivers/gpio/gpio_stm32.c
  ↓
Vendor HAL     STM32Cube low-level code
  ↓
Hardware       GPIO register
```

**Rule:** always use the Zephyr API. Never call the vendor HAL or registers
directly. The driver handles the hardware details for you.

---

## 16. Common problems

| Problem | Fix |
|---|---|
| `west: command not found` | Activate the venv: `source ~/zephyrproject/.venv/bin/activate` |
| `find_package(Zephyr) not found` | Run `west zephyr-export` |
| Strange build errors | Clean build: `west build -p always ...` |
| Flash fails / permission denied | Install the missing tool or fix udev rules |
| Blinky not supported on the board | Use `samples/hello_world` instead |
| `west update` is slow or huge | Use manifest groups to skip unused modules |

---

## 17. Cheat sheet

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject

west --version
west update                    # update all projects
west boards | grep <name>      # find a board
west build -p always -b <board> <app>
west build -t run              # run in simulation
west flash                     # flash the board
west build -t menuconfig       # edit Kconfig
west debug                     # start debugging
```
