# ITI-Zephyr-Playground

Hands-on Zephyr RTOS course: from zero to a custom out-of-tree module and a
custom Devicetree on real hardware.

> **Main reference:** the official
> [Zephyr documentation](https://docs.zephyrproject.org/). Use it for every API,
> Kconfig symbol, board, and Devicetree binding. This repo is a guided path
> through that documentation.

Every lesson uses the standard project layout:

```
my_project/
├── src/        ← your .c files
├── inc/        ← your .h files (optional)
├── build/      ← build output (generated, do not commit)
├── CMakeLists.txt
└── prj.conf
```

The build runs **inside the project folder** (`west build -d build .`), so the
`build/` output stays next to your code — even when the Zephyr workspace is in
another location.

## Learning path

| # | Folder | What you learn | Companion guide |
|---|---|---|---|
| 01 | `01_Introduction_To_Zephyr` | What Zephyr is, history, architecture, why an RTOS | — |
| 02 | `02_Installation_Guide_What_Gets_Installed` | Ubuntu install (`west init/update`, SDK) + what lands on disk | — |
| 03 | `03_Create_New_Project` | Create apps from scratch: the `new_project` template (`src/inc/build`), Black Pill `blackpill_f401cc` GPIO blinky + ESP32-S3 Mini `weact_esp32s3_mini/esp32s3/procpu` RGB LED, build/flash steps | [Using Drivers and Software Modules](03_Create_New_Project/Using_Drivers_And_Software_Modules.md) |
| 04 | `04_CMake` | How CMake builds a Zephyr app: the 5 important lines in `CMakeLists.txt`, `src/inc/cmake/build`, project-local builds | — |
| 05 | `05_Kconfig_vs_Devicetree_DTS` | Kconfig (what code) vs Devicetree (which hardware), how they meet | [Devicetree Overlays Guide](<08_Devicetree/Devicetree_Overlays_Guide.md>), [Build Outputs](<05_Kconfig_vs_Devicetree_DTS/Build_Configuration_Outputs.md>) |
| 06 | `06_KConfig` | Kconfig deep dive: symbols, `depends on`/`select`/`default`, priority hierarchy, `menuconfig`, `.config` → `zephyr/autoconf.h`, demo app + random-number example | [Finding Features and Kconfig](06_KConfig/Finding_Features_And_Kconfig.md) |
| 07 | `07_Create_Custom_Module_Digital_To_Volt` | Custom module end-to-end: `digital_to_volt_Module/` (`module.yml`, `Kconfig`, `CMakeLists`, `digital_to_volt.h/.c`) + `app/` demo (simulate on `native_sim`, real ADC on PA1 + LED threshold) | — |
| 08 | `08_Devicetree` | Devicetree deep dive: concepts, syntax, bindings, macros, overlays, generated files | [Devicetree Overlays Guide](<08_Devicetree/Devicetree_Overlays_Guide.md>) |
| 09 | `09_Create_Custom_Devicetree` | Create a custom binding (`power-switch`) and use it — Case A (auto-discovered in `app/dts/bindings/`) and Case B (separate DTS root, inside or outside Zephyr) | — |
| 10 | `10_How_To_Create_A_Zephyr_Project` | Summary of the whole creation process: 9 steps from board to flashed firmware | — |

Suggested order: **01 → 02 → 03 → 04 → 05 → 06 → 08 → 07 / 09**.

> `07` (custom module) and `09` (custom Devicetree) are independent advanced
> topics; do them in either order after `08`.

---

## Important commands

Run these from **your project folder** (with `ZEPHYR_BASE` set) or from the
Zephyr workspace.

### Environment (every new terminal)

```bash
source ~/zephyrproject/.venv/bin/activate
export ZEPHYR_BASE=~/zephyrproject/zephyr
```

### Build

```bash
west build -b <board> -d build .            # build into ./build
west build -p always -b <board> -d build .  # clean build
west build -t menuconfig -d build           # edit Kconfig (terminal)
west build -t guiconfig -d build            # edit Kconfig (graphical)
west build -t run -d build                  # run on native_sim / QEMU
```

### Flash

```bash
west flash -d build                         # flash the board
west flash -d build --runner <runner>       # choose a specific runner
```

### Inspect the result

```bash
cat build/zephyr/.config                    # final Kconfig
cat build/zephyr/zephyr.dts                 # final merged Devicetree
ls build/zephyr/zephyr.elf                  # the firmware
```

### Discovery

```bash
west boards | grep -i <name>                # find a board target
west --version                              # west version
cmake --version                             # CMake version
ninja --version                             # Ninja version
```

> **Order tip:** activate the venv first, then set `ZEPHYR_BASE`, then build.
> If you open a new terminal, do the first two steps again.

---

## Repository layout

```
01_Introduction_To_Zephyr/
02_Installation_Guide_What_Gets_Installed/
03_Create_New_Project/          ← new_project template + 2 board examples
04_CMake/
05_Kconfig_vs_Devicetree_DTS/
06_KConfig/                     ← deep dive + random_demo example
07_Create_Custom_Module_Digital_To_Volt/
08_Devicetree/
09_Create_Custom_Devicetree/
```
