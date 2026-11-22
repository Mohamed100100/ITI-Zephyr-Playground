# Zephyr Build Configuration Outputs

> Companion guide to [Kconfig vs Devicetree](README.md),
> [Devicetree deep dive](../07_Devicetree/README.md), and
> [Kconfig deep dive](../05_KConfig/README.md).
>
> Use this when you want to understand **what the build actually produced** and
> where to look to verify your hardware description and software settings.

---

## Overview

During the build, Zephyr resolves two kinds of configuration:

| Configuration | Main inputs | Final readable output |
|---|---|---|
| **Hardware description** | Board devicetree, included files, overlays | `build/zephyr/zephyr.dts` |
| **Software configuration** | Kconfig rules, board defaults, application settings | `build/zephyr/.config` |

These files show **what the build actually used**, rather than only what your
application requested.

---

## Table of Contents

1. [Final hardware description: `zephyr.dts`](#1-final-hardware-description-zephyrdts)
2. [Final software configuration: `.config`](#2-final-software-configuration-config)
3. [How they reach the firmware](#3-how-they-reach-the-firmware)
4. [How to use these outputs](#4-how-to-use-these-outputs)
5. [Final conclusion](#final-conclusion)

---

## 1. Final Hardware Description: `zephyr.dts`

### Simplified Flow

```text
Board .dts
    + recursively included .dtsi files
    + selected overlays
    → combined and processed devicetree
    → build/zephyr/zephyr.dts
```

This is **merging and processing**, not simply concatenating files.

### Role of Each Input

- **Board `.dts`**: describes the board’s hardware.
- **Included `.dtsi` files**: provide shared hardware definitions, such as SoC
  controllers.
- **Application overlay**: adds or changes hardware information for your project.

### What the Result Contains

The final tree describes hardware information such as:

- GPIO controllers.
- UART and SPI instances.
- Enabled or disabled devices.
- Pin connections and routing.
- LED connections.
- Aliases and chosen devices.

### Example

If your overlay describes an external LED on GPIO7, the final `zephyr.dts`
should contain that LED node and its resolved GPIO connection.

**An overlay describes hardware; it does not execute operations or turn the LED
on.**

---

## 2. Final Software Configuration: `.config`

### Simplified Flow

```text
Kconfig definitions and rules
    + board/SoC defaults
    + board *_defconfig
    + application prj.conf
    + extra configuration fragments, if supplied
    → resolve defaults, dependencies, and selections
    → build/zephyr/.config
```

### Role of Each Input

- **Kconfig files**: define available options, types, defaults, and dependencies.
- **Board defaults**: supply initial settings for the selected target.
- **`prj.conf`**: requests settings needed by your application.
- **Extra fragments**: supply additional settings when included in the build.

### What the Result Contains

`.config` records the **final resolved option values**.

For example:

```ini
CONFIG_GPIO=y
# CONFIG_LED_STRIP is not set
```

This means GPIO support is enabled, while LED-strip support is disabled.

**An option appearing in `.config` does not mean its module is enabled or
compiled. Check its value.**

Zephyr also does not automatically enable a feature simply because your C code
calls its API.

---

## 3. How They Reach the Firmware

The build generates C configuration headers from the resolved information:

```text
Hardware description → generated devicetree macros
Software configuration → generated CONFIG_ macros
                                 │
                                 ▼
                  Compile application and selected sources
                                 │
                                 ▼
                         Link the firmware
```

The firmware normally does **not** read `.dts`, overlays, or `prj.conf` as text
at startup.

---

## 4. How to Use These Outputs

| Question | File to inspect |
|---|---|
| Did my overlay add the LED correctly? | `build/zephyr/zephyr.dts` |
| Is the intended hardware node enabled? | `build/zephyr/zephyr.dts` |
| Was GPIO support enabled? | `build/zephyr/.config` |
| Did my requested software setting take effect? | `build/zephyr/.config` |

**Do not edit generated outputs as permanent configuration.**

Make changes in:

- Your **overlay** for hardware configuration.
- Your **`prj.conf`** for application software settings.

Then rebuild and inspect the results.

---

## Final Conclusion

> **`zephyr.dts` describes the final hardware configuration.**
> **`.config` records the final software configuration.**
> **Application code uses the enabled APIs to operate that hardware or
> software.**

---

## Related documents

- [Kconfig vs Devicetree](README.md) — side-by-side comparison.
- [Devicetree deep dive](../07_Devicetree/README.md) — concepts, syntax,
  bindings, macros.
- [Devicetree Overlays Guide](../07_Devicetree/Devicetree_Overlays_Guide.md)
  — where to find overlay properties and how to write overlays.
- [Kconfig deep dive](../05_KConfig/README.md) — the configuration system.
- [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md)
  — how to find the right `CONFIG_...` option.
- [Create a New Zephyr Project](../03_Create_New_Project/README.md) — full
  project examples.
