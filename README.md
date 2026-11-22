# ITI-Zephyr-Playground

Hands-on Zephyr RTOS course: from zero to a custom out-of-tree module and a
custom Devicetree on real hardware.

## Learning path

| # | Folder | What you learn | Companion guide |
|---|---|---|---|
| 01 | `01_Introduction_To_Zephyr` | What Zephyr is, history, architecture, why an RTOS | — |
| 02 | `02_Installation_Guide_What_Gets_Installed` | Ubuntu install (`west init/update`, SDK) + what lands on disk | — |
| 03 | `03_Create_New_Project` | Create apps from scratch: Black Pill `blackpill_f401cc` GPIO blinky + ESP32-S3 Mini `weact_esp32s3_mini/esp32s3/procpu` RGB LED, build/flash steps | [Using Drivers and Software Modules](03_Create_New_Project/Using_Drivers_And_Software_Modules.md) |
| 04 | `04_Kconfig_vs_Devicetree(DTS)` | Kconfig (what code) vs Devicetree (which hardware), how they meet | [Devicetree Overlays Guide](<07_Devicetree/Devicetree_Overlays_Guide.md>) |
| 05 | `05_KConfig` | Kconfig deep dive: symbols, `depends on`/`select`/`default`, priority hierarchy, `menuconfig`, `.config` → `zephyr/autoconf.h`, demo app | [Finding Features and Kconfig](05_KConfig/Finding_Features_And_Kconfig.md) |
| 06 | `06_Create_Custom_Module_(Digital_To_Volt)` | Custom module end-to-end: `digital_to_volt_Module/` (`module.yml`, `Kconfig`, `CMakeLists`, `digital_to_volt.h/.c`) + `app/` demo (simulate on `native_sim`, real ADC on PA1 + LED threshold) | — |
| 07 | `07_Devicetree` | Devicetree deep dive: concepts, syntax, bindings, macros, overlays, generated files | [Devicetree Overlays Guide](<07_Devicetree/Devicetree_Overlays_Guide.md>) |
| 08 | `08_Create_Custom_Devicetree` | Create a custom binding (`power-switch`) and use it — Case A (auto-discovered in `app/dts/bindings/`) and Case B (separate DTS root, inside or outside Zephyr) | — |

Suggested order: **01 → 02 → 03 → 04 → 05 → 07 → 06 / 08**.

> `06` and `08` are independent advanced topics; do them in either order after `07`.
