# ITI-Zephyr-Playground

Hands-on Zephyr RTOS course: from zero to a custom out-of-tree module on real hardware.

| # | Folder | What you learn |
|---|---|---|
| 01 | `01_Introduction_To_Zephyr` | What Zephyr is, history, architecture, why an RTOS |
| 02 | `02_Installation_Guide_What_Gets_Installed` | Ubuntu install (`west init/update`, SDK) + what lands on disk |
| 03 | `03_Create_New_Project` | New apps from scratch: Black Pill `blackpill_f401cc` GPIO blinky + ESP32-S3 Mini `weact_esp32s3_mini/esp32s3/procpu` RGB LED, with full build/flash steps |
| 04 | `04_Kconfig_vs_Devicetree(DTS)` | Kconfig (what code) vs Devicetree (which hardware), how they meet |
| 05 | `05_KConfig` | **Kconfig deep dive only**: symbols, `depends on`/`select`/`default`, priority hierarchy, `menuconfig`, `.config` → `autoconf.h`, demo app |
| 06 | `06_Create_Custom_Module_(Digital_To_Volt)` | **Custom module end-to-end**: `d2v_module/` (`module.yml`, `Kconfig`, `CMakeLists`, `digital_to_volt.h/.c`) + `app/` demo (simulate on `native_sim`, real ADC on PA1 + LED threshold) + build/flash commands |

Suggested order: 01 → 02 → 03 → 04 → 05 → 06.
