# Zephyr Project — Complete Installation Guide & What Gets Installed

> **Host:** Ubuntu 24.04 LTS+ | **Zephyr:** v4.4.0-rc1 | **Zephyr SDK:** 1.0.0 | **Language:** English
> **Official guide followed:** https://docs.zephyrproject.org/latest/develop/getting_started/index.html
>
> **Workspace root used below:** `~/zephyrproject`


This README has two parts:

- **Part 1 — How to install:** every command, in order, with an explanation of what
  each step does and how to verify it.
- **Part 2 — What is installed:** a full tour of everything that lands on disk after
  the installation — the workspace layout, the Zephyr OS itself, every external
  module, the SDK toolchains, and how it all fits together.

---

# PART 1 — HOW TO INSTALL

## 1.1 What you are building

A **West workspace** — not a single app. It is a Zephyr RTOS checkout plus all
external dependencies (vendor HALs, libraries, the MCUboot bootloader, tools)
pinned by a manifest file and managed by the `west` meta-tool.

```
~/zephyrproject/                ← the West workspace (everything below lives here)
```

---

## 1.2 Prerequisites (Ubuntu 24.04 LTS+)

Minimum tool versions required by Zephyr:

| Tool | Minimum version | How to check |
|---|---|---|
| CMake | 3.28.0 | `cmake --version` |
| Python | 3.12 (strongly recommended) | `python3 --version` |
| Devicetree compiler (dtc) | 1.4.6 | `dtc --version` |
| Git, Ninja, gperf, ccache, etc. | — | installed via apt below |

### Step 1 — Update the OS

```bash
sudo apt update
sudo apt upgrade
```

### Step 2 — Install host dependencies

```bash
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3-dev python3-venv python3-tk \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
```

What each package is for:

| Package | Purpose |
|---|---|
| `git` | cloning Zephyr and west-managed projects |
| `cmake` | build system generator |
| `ninja-build` | the actual builder Zephyr uses |
| `gperf` | perfect-hash table generator used by some Zephyr components |
| `ccache` | compiler cache — speeds up rebuilds |
| `dfu-util` | flashing STM32 boards over USB DFU (e.g. Black Pill) |
| `device-tree-compiler` | `dtc` — validates devicetree files |
| `wget`, `xz-utils`, `file` | downloading and extracting SDK/tooling archives |
| `python3-dev`, `python3-venv`, `python3-tk` | Python venv + headers for `west` and build scripts |
| `make`, `gcc`, `gcc-multilib`, `g++-multilib` | host compiler bits needed by native builds |
| `libsdl2-dev` | display support for `native_sim` graphical apps |
| `libmagic1` | file-type detection used by some scripts |

> **ARM64 (AArch64) note:** `gcc-multilib` / `g++-multilib` don't exist on ARM64 —
> omit them from the list.

### Step 3 — Verify prerequisites

```bash
cmake --version   # >= 3.28.0
python3 --version # 3.12.x preferred
dtc --version     # >= 1.4.6
```

Other Linux distros: see
https://docs.zephyrproject.org/latest/develop/getting_started/installation_linux.html

---

## 1.3 Installation — step by step

All commands assume `~/zephyrproject` as the workspace root. If your workspace
lives elsewhere (e.g. on another drive), substitute the path.

### Step 4 — Create and activate a Python virtual environment

This isolates `west` and Zephyr's Python dependencies from your system Python.

```bash
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
# your prompt now shows: (.venv) user@host:...
```

> ⚠️ **Activate this venv in every new terminal before using `west`:**
> ```bash
> source ~/zephyrproject/.venv/bin/activate
> ```
> Deactivate anytime with `deactivate`.

### Step 5 — Install `west`

West is Zephyr's workspace manager — it clones, updates, builds, flashes, and debugs.

```bash
pip install west
west --version
```

### Step 6 — Get the Zephyr source (`west init` + `west update`)

```bash
west init -m https://github.com/zephyrproject-rtos/zephyr ~/zephyrproject
cd ~/zephyrproject
west update
```

**What just happened:**

1. `west init` marked `~/zephyrproject` as a west workspace by creating `.west/config`:

   ```ini
   [manifest]
   path = zephyr
   file = west.yml
   ```

   It also cloned the `zephyr` repository itself — this is the **manifest repository**.

2. `west update` read `zephyr/west.yml` — a manifest listing ~70 external projects,
   each pinned to an exact `revision:` (a git hash). West cloned every one of them
   into `modules/*`, `bootloader/*`, and `tools/*`.

> 💡 To save disk / skip unused vendor HALs, look at **Project Groups** before
> running `west update`:
> https://docs.zephyrproject.org/latest/develop/west/manifest.html#west-manifest-groups

### Step 7 — Install Zephyr Python dependencies

```bash
west packages pip --install
```

This reads the requirement files from the *checked-out* workspace, so the package
versions match this exact Zephyr version. It may upgrade or downgrade `west`
itself — that is normal.

### Step 8 — Export the Zephyr CMake package

```bash
west zephyr-export
```

This registers this Zephyr checkout in CMake's user package registry, so
`find_package(Zephyr)` finds it automatically whenever you build an application —
no need to set `ZEPHYR_BASE` by hand for every build.

### Step 9 — Install the Zephyr SDK

The SDK contains the cross-compilers (gcc), QEMU, OpenOCD, and host tools for
every supported architecture. (`zephyr/SDK_VERSION` here says `1.0.0`.)

```bash
cd ~/zephyrproject/zephyr
west sdk install
```

Useful options:

```bash
west sdk install --help
west sdk install --install-dir ~/my-sdk        # custom install location
west sdk install --toolchains arm-zephyr-eabi  # only one architecture
```

Without `west sdk`, you can install it manually:
https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html

### Step 10 (later) — How to update everything

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject
west update                  # fetch all pinned revisions
west packages pip --install  # re-sync python deps
west zephyr-export           # re-register cmake package
```

---

## 1.4 Verify the installation (smoke tests)

Always start every session with:

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject/zephyr
```

### List supported boards

```bash
west boards | less
west boards | grep -i -E "qemu|native|nucleo|nrf|esp32"
```

Board catalog (with exact `-b` names): https://docs.zephyrproject.org/latest/boards/index.html

> Multi-core boards need a SoC/core suffix, e.g. `nrf5340dk/nrf5340/cpuapp`.
> See https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html#board-terminology

### Test A — simulation (no hardware)

**A1. `hello_world` on `native_sim` (fastest smoke test):**

```bash
west build -p always -b native_sim samples/hello_world
west build -t run
# Expected: *** Booting Zephyr OS ... *** Hello World! native_sim
```

**A2. `blinky` on QEMU:**

```bash
west build -p always -b qemu_x86 samples/basic/blinky
west build -t run
# or: west build -b qemu_cortex_m3 samples/basic/blinky && west build -t run
```

`blinky` on QEMU has no physical LED — the console toggling or a `native_sim`
LED sim is what you watch. For `hello_world`, printed output is the pass criterion.

Useful variants:

```bash
west build -p auto -b native_sim samples/hello_world  # reuse build cache after first pristine build
west build -t run -- --help                           # runner options
rm -rf build                                          # manual clean (same as -p always next build)
```

### Test B — real hardware board

1. **Pick a board name**, e.g. `nucleo_f401re`, `esp32_devkitc_wroom`, `nrf52840dk/nrf52840`:

   ```bash
   west boards | grep -i nucleo
   ```

2. **Build:**

   ```bash
   west build -p always -b <your-board-name> samples/basic/blinky
   # fallback if blinky unsupported on that board:
   west build -p always -b <your-board-name> samples/hello_world
   ```

   Blinky requirement: the board must expose a GPIO LED via the devicetree
   `led0` alias. Check `zephyr/boards/<vendor>/<board>/*.dts` or the board docs page.

3. **Connect the board via USB**, power it on, and check its docs page for jumpers/boot mode:
   https://docs.zephyrproject.org/latest/boards/index.html

4. **Flash:**

   ```bash
   west flash
   ```

   If `west flash` complains about missing host tools, install the one it names
   (OpenOCD, J-Link, dfu-util, esptool...):
   https://docs.zephyrproject.org/latest/develop/flash_debug/host-tools.html

   **Linux udev rules (first time with a debug probe):**
   https://docs.zephyrproject.org/latest/develop/beyond-GSG.html#setting-udev-rules
   (copy Zephyr's udev rules for your probe, then `sudo udevadm control --reload-rules`.)

5. **Expected result:**
   * `blinky`: on-board LED blinks (~1 Hz). See `samples/basic/blinky/src/main.c` —
     it toggles `led0` with `gpio_pin_toggle()`.
   * `hello_world`: open a serial monitor at the board's baud (usually 115200 8N1):
     ```bash
     pip install pyserial
     python -m serial.tools.miniterm /dev/ttyACM0 115200
     # or: west espressif monitor, picocom, screen, minicom
     ```

### Test B1 — WeAct Black Pill (this workspace's board)

Zephyr supports 5 variants under `zephyr/boards/weact/`. Pick the one matching
the chip printed on your board:

| Your silkscreen | Zephyr `-b` name | SoC | Flash / SRAM | Notes |
|---|---|---|---|---|
| Black Pill V3.0 (most common F401) | `blackpill_f401ce` | STM32F401CEU6 Cortex-M4F 84 MHz | 512 KB / 96 KB | USB-C, 25 MHz HSE |
| Black Pill V1.2 | `blackpill_f401cc` | STM32F401CCU6 Cortex-M4F 84 MHz | 256 KB / 64 KB | older V1.2 |
| Black Pill V2.0 (most common F411) | `blackpill_f411ce` | STM32F411CEU6 Cortex-M4F 100 MHz | 512 KB / 128 KB | 96 MHz sysclk for stable USB |
| Black Pill U585 | `blackpill_u585ci` | STM32U585CI Cortex-M33 | 2 MB / 786 KB | TrustZone board |
| Black Pill H523 | `blackpill_h523ce` | STM32H523CE Cortex-M33 | 512 KB / 256 KB | newer H5 family |

Check yours:

```bash
west boards | grep -i blackpill
# blackpill_f401ce, blackpill_f401cc, blackpill_f411ce, blackpill_h523ce, blackpill_u585ci
```

Default Zephyr peripheral mapping (same on F401CE/F411CE — see `blackpill_f411ce.dts` / `doc/index.rst`):

* `USER_LED` = `PC13` (active-low, alias `led0` — what blinky toggles)
* `USER_PB` = `PA0` (alias `sw0`)
* `UART_1 TX/RX` = `PA9/PA10` @ 115200 (`zephyr,console` — hello_world prints here, needs a USB-TTL dongle)
* `I2C1 SCL/SDA` = `PB8/PB9`, `SPI1` = `PA4/PA5/PA6/PA7`, `ADC1` = `PA1`, `PWM4_CH1/CH2` = `PB6/PB7`
* Clocks: 25 MHz HSE + 32.768 kHz LSE, PLL → 84 MHz (F401) / 96 MHz (F411)

Build (F411CE example — replace with your variant):

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject/zephyr
west build -p always -b blackpill_f411ce samples/basic/blinky
# F401CE version:
# west build -p always -b blackpill_f401ce samples/basic/blinky
```

Flash via the factory ROM DFU bootloader (no ST-Link needed):

1. Install `dfu-util >= 0.8` (the Ubuntu package is old — build from source if `west flash` fails):
   ```bash
   sudo apt install dfu-util
   dfu-util --version
   ```
2. Plug in USB-C. Force DFU mode: **hold BOOT0, press+release NRST, then release BOOT0**.
   The board re-enumerates as `0483:df11 STM32 DFU`.
   Verify with `lsusb | grep -i dfu` or `dfu-util -l`.
3. Flash:
   ```bash
   west flash
   # equivalent manual command:
   # dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D build/zephyr/zephyr.bin
   ```
4. Press NRST once to boot the app. The on-board LED (PC13) should blink.

Alternative with an SWD probe (ST-Link/J-Link): solder a 4-pin header
(3V3, GND, SWCLK, SWDIO), then `west flash --runner openocd` or `--runner jlink`.
See `doc/index.rst` → Debugging.

Serial console: the Black Pill's USB-C is **power + DFU only**, not a UART. To see
`hello_world` output, wire `PA9/PA10` to a USB-TTL adapter (115200 8N1) + GND, then:

```bash
python -m serial.tools.miniterm /dev/ttyUSB0 115200
```

---

# PART 2 — WHAT IS INSTALLED

After Part 1, your disk contains a complete Zephyr development environment. Here is
everything that was installed and what each piece is for.

## 2.1 The big picture — the layered software model

Zephyr is a **scalable real-time operating system (RTOS)** for resource-constrained
devices — from sensors and LED wearables to smart watches and IoT gateways
(see `zephyr/README.rst`).

When your application runs, the calls flow through four layers:

```
Your app (samples/basic/blinky/src/main.c)
  -> Zephyr API            (zephyr/include/zephyr/drivers/gpio.h : gpio_pin_toggle())
    -> Zephyr driver       (zephyr/drivers/gpio/gpio_stm32.c)
      -> Vendor HAL        (modules/hal/stm32/stm32cube/.../stm32_ll_gpio.h : LL_GPIO_SetPinMode())
        -> Hardware        (GPIO->MODER register)
```

> **Rule:** application code never calls `modules/hal/` directly. Always use Zephyr
> driver APIs. The Zephyr driver translates generic calls + devicetree into HAL
> calls and handles kernel integration (IRQ, power management, logging, `struct device`).

Example — `zephyr/drivers/gpio/gpio_stm32.c`:

```c
#define DT_DRV_COMPAT st_stm32_gpio
#include <zephyr/drivers/gpio.h>
#include <stm32_ll_gpio.h>  // <-- comes from modules/hal/stm32
```

### What each layer gives you

| Layer | Where | What it provides |
|---|---|---|
| Small-footprint preemptive kernel | `zephyr/kernel/` | threads, scheduling, mutexes, semaphores, queues, timers |
| Portable driver + subsystem APIs | `zephyr/drivers/`, `zephyr/subsys/` | GPIO, UART, I2C, SPI, USB, BLE, Networking, FS, Power Mgmt |
| Build system | CMake + Kconfig + Devicetree | `CMakeLists.txt` + `prj.conf` (Kconfig) + `*.dts`/`*.overlay` (pins/peripherals) |
| Arch / SoC support | `zephyr/arch/`, `zephyr/soc/` | ARM Cortex-M/A/R, x86, ARC, RISC-V, Xtensa, SPARC, MIPS |
| Board support | `zephyr/boards/<vendor>/<board>/` | 160+ vendors, 1000s of boards — plus `zephyr/boards/qemu/` and `zephyr/boards/native/` for simulation |
| Samples & tests | `zephyr/samples/`, `zephyr/tests/` | `samples/basic/blinky`, `samples/hello_world`, etc. |
| Vendor HALs (external) | `modules/hal/*` | ST STM32Cube, Nordic nrfx, Espressif, NXP, ... — raw register code |
| Secure bootloader | `bootloader/mcuboot/` | MCUboot — signed OTA / secure boot |
| SDK toolchains | installed via `west sdk install` | gcc + QEMU + OpenOCD per-arch builds |

---

## 2.2 Top level of the workspace (`~/zephyrproject/`)

```
zephyrproject/
├── zephyr/       # MAIN REPO — kernel, drivers, arch, soc, boards, samples. See §2.3
├── modules/      # EXTERNAL west projects from zephyr/west.yml. See §2.4
├── bootloader/   # MCUboot secure bootloader. See §2.5
├── tools/        # Test / net helpers. See §2.6
├── .west/config  # Marks this dir as a west workspace (manifest = zephyr/west.yml)
├── .venv/        # Python venv (west + build scripts). Never commit.
├── .vscode/      # Editor settings (optional)
└── build/        # Default west build output (created after the first `west build`)
```

---

## 2.3 `zephyr/` — the operating system itself

This is the main repository — the manifest repo that `west init` cloned first.

| Path | Content |
|---|---|
| `arch/` | Per-CPU-architecture context switch, interrupts, boot code (arm, x86, riscv, arc, xtensa, ...) |
| `soc/` | Per-SoC-family init, clock, pinmux (st_stm32, nordic_nrf, espressif_esp32, ...) |
| `boards/` | Per-board devicetree + Kconfig + docs. 160+ vendors (`st/`, `nordic/`, `espressif/`, `qemu/`, `native/`...). Find yours with `west boards` |
| `drivers/` | **Zephyr-side drivers** — portable wrappers implementing the driver APIs. E.g. `drivers/gpio/gpio_stm32.c`, `drivers/serial/uart_nrfx_uarte.c`, `drivers/serial/uart_ambiq.c`. These use `DT_DRV_COMPAT`, `DEVICE_DT_INST_DEFINE`, and handle IRQ + power management |
| `dts/` | Devicetree bindings + includes (`dts/bindings/gpio/st,stm32-gpio.yaml`, `dts/arm/...`) |
| `include/zephyr/` | Public C API: `drivers/gpio.h`, `kernel.h`, `device.h`, `logging/log.h`, `sys/*` |
| `kernel/` | Scheduler, threads, IPC (mutex/sem/queue/fifo/lifo/stack), timers, work queues |
| `subsys/` | High-level stacks: `bluetooth/`, `net/`, `usb/`, `fs/`, `logging/`, `shell/`, `dfu/`, `mgmt/` |
| `lib/` | Internal libs: `libc`, `os/`, `utils/`, crypto helpers |
| `samples/` | Demo apps. Start with `samples/basic/blinky` (GPIO LED) and `samples/hello_world` (console print) |
| `tests/` | Twister/pytest test suite (`twister`, `ztest`) |
| `scripts/` | `west-commands.yml`, `west build/flash` helpers, `dts/` and `kconfig/` tooling |
| `cmake/`, `CMakeLists.txt`, `Kconfig`, `Kconfig.zephyr` | Build + configuration system |
| `doc/` | Sources for docs.zephyrproject.org |
| `west.yml` | **The manifest** — pins every external project to an exact revision under `projects:` + `self: path: zephyr` |
| `VERSION` | Here: `4.4.0-rc1` (`VERSION_MAJOR=4 MINOR=4 PATCHLEVEL=0 EXTRAVERSION=rc1`) |
| `SDK_VERSION` | Minimum SDK expected (`1.0.0` here) |
| `zephyr-env.sh` / `zephyr-env.cmd` | Legacy env setup scripts (prefer `west zephyr-export` + venv) |
| `submanifests/` | Extra manifest fragments imported by `west.yml` |

---

## 2.4 `modules/` — external dependencies (cloned by `west update`)

These are the ~70 projects pinned in `zephyr/west.yml`. Each has its own git
history and a `zephyr/module.yml` telling Zephyr how to integrate it.

```
modules/
├── hal/     # Vendor HALs — one directory per vendor, upstream code untouched:
│            #   st/ stm32/ (STM32Cube LL/HAL), nordic/ (nrfx), espressif/,
│            #   nxp/, silabs/, infineon/, microchip/, ambiq/, cmsis/, cmsis_6/, ...
├── lib/     # Third-party libraries: gui/lvgl, cmsis-dsp/nn, nanopb, zcbor, open-amp,
│            #   openthread, picolibc, hostap, liblc3, uoscore-uedhoc, ...
├── fs/      # Filesystems: fatfs/, littlefs/
├── crypto/  # mbedtls/, mbedtls-3.6/, tf-psa-crypto/, mldsa-native/
├── debug/   # segger/, percepio/, mipi-sys-t/
├── tee/     # Trusted execution: tf-m/trusted-firmware-m, tf-a/trusted-firmware-a, ...
└── bsim_hw_models/ # nRF hardware simulation models for BabbleSim
```

You rarely edit anything here — `west update` refreshes these to the pinned
hashes from the manifest.

---

## 2.5 `bootloader/mcuboot/`

**MCUboot** — an MCU bootloader for secure boot + signed OTA/DFU updates.
Only needed if your product uses a bootloader with image signing. Cloned as the
west project `mcuboot`.

---

## 2.6 `tools/`

| Path | Purpose |
|---|---|
| `tools/net-tools` | Network test helpers |
| `tools/edtt` | Bluetooth PTS / EDTT test scripts |
| `tools/bsim/` | BabbleSim BLE/physics simulator components (only if the `babblesim` group is enabled) |

---

## 2.7 Hidden and generated paths

| Path | Purpose |
|---|---|
| `.west/config` | `path=zephyr file=west.yml` + `base=zephyr`. Do not edit by hand — managed by `west init` / `west config` |
| `.venv/` | Python venv binaries + pip-installed packages. Recreate with `python3 -m venv` if broken |
| `zephyr/build/` or `<app>/build/` | CMake + Ninja output: `zephyr.elf`, `zephyr.hex`, `zephyr.bin`, `compile_commands.json`, `.config`, `devicetree_generated.h`. Delete it or use `west build -p always` for a pristine rebuild |

---

## 2.8 The Zephyr SDK (installed by `west sdk install`)

Separate from the source tree, the SDK provides the **toolchains**:

| Component | Purpose |
|---|---|
| `arm-zephyr-eabi-gcc` (and per-arch variants) | cross-compilers for ARM, x86, RISC-V, ARC, Xtensa, ... |
| QEMU | emulate boards without hardware (`qemu_x86`, `qemu_cortex_m3`, ...) |
| OpenOCD | flash + debug via SWD/JTAG probes |
| host tools | supporting utilities for builds and runners |

---

## 2.9 Anatomy of a sample app (what the tests run)

* `samples/basic/blinky/CMakeLists.txt` — `find_package(Zephyr)` + `target_sources(app PRIVATE src/main.c)`
* `samples/basic/blinky/prj.conf` — often empty or just `CONFIG_GPIO=y`
* `samples/basic/blinky/src/main.c` — `GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios)` → `gpio_pin_configure()` → loop of `gpio_pin_toggle()` + `k_msleep()`
* `samples/basic/blinky/sample.yaml` — Twister metadata (which boards CI tests)
* `samples/basic/blinky/boards/` — per-board overlays if the LED alias differs

---

## 2.10 Troubleshooting

| Symptom | Fix |
|---|---|
| `west: command not found` | `source ~/zephyrproject/.venv/bin/activate`, then `pip install west` |
| Stale build / Kconfig weirdness | `west build -p always ...` (pristine build) or `rm -rf build` |
| `find_package(Zephyr) not found` | run `west zephyr-export`; ensure `ZEPHYR_BASE=~/zephyrproject/zephyr` |
| `west flash` missing tool / permission denied | install the host tool named in the error; fix udev rules; use `sudo` only for diagnosis — prefer udev rules |
| Blinky not supported on board | use `samples/hello_world` instead (as the Getting Started guide does) |
| Python version errors on newer Python | use the Python 3.12 venv as the guide mandates |
| `west update` huge / slow | enable shallow clones / groups: `west config manifest.group-filter ...` — see west manifest docs |

---

## 2.11 Cheat sheet

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject

west --version
west status                    # git status of all west projects
west update                    # fetch pinned revisions
west packages pip --install    # sync python deps
west boards | grep <name>      # find a board target
west build -p always -b <board> zephyr/samples/hello_world
west build -t run              # run on native_sim/qemu
west flash                     # flash connected board
west debug                     # attach debugger (openocd/jlink)
west build -t menuconfig       # Kconfig UI for the current build
west build -t guiconfig        # graphical Kconfig UI
```
---

