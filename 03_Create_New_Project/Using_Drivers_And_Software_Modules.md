# Using Drivers and Software Modules in Zephyr

> Companion guide to [Create a New Zephyr Project](README.md),
> [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md), and
> [Devicetree Overlays Guide](<../04_Kconfig_vs_Devicetree(DTS)/Devicetree_Overlays_Guide.md>).
>
> This guide explains the general lifecycle for using any Zephyr feature:
> hardware drivers, subsystems, and software modules.
>
> **Learning path:** `03 Create Project` → `04 Kconfig vs Devicetree` →
> `05 Kconfig deep dive` → `06 Custom Module`.

---

## Overview

To use a feature in your application:

> **Enable it → describe its hardware if needed → read its API → obtain its
> device or object → check readiness → configure → operate → handle errors.**

**Not every feature follows exactly the same lifecycle.** Hardware drivers and
software modules can have different initialization requirements.

---

## Table of Contents

1. [Identify the feature and public API](#1-identify-the-feature-and-public-api)
2. [Enable the required software](#2-enable-the-required-software)
3. [Describe the hardware when required](#3-describe-the-hardware-when-required)
4. [Understand initialization](#4-understand-initialization)
5. [Obtain the device or object](#5-obtain-the-device-or-object)
6. [Check readiness](#6-check-readiness)
7. [Configure the feature for your application](#7-configure-the-feature-for-your-application)
8. [Perform operations and handle results](#8-perform-operations-and-handle-results)
9. [Build and verify](#9-build-and-verify)
10. [Example workflow: external GPIO LED](#example-workflow-external-gpio-led)
11. [Final checklist](#final-checklist)

---

## 1. Identify the Feature and Public API

Start with the feature’s **public header**, API documentation, and a basic
sample.

Common headers:

| Feature | Header included by the application |
|---|---|
| GPIO | `zephyr/drivers/gpio.h` |
| UART | `zephyr/drivers/uart.h` |
| Addressable LED strip | `zephyr/drivers/led_strip.h` |
| Logging | `zephyr/logging/log.h` |

Read the API to discover:

- Required structures and objects.
- Initialization or configuration functions.
- Available operations.
- Function arguments.
- Return values and error conditions.

**Use public APIs, not internal driver functions.**

---

## 2. Enable the Required Software

Find the feature’s Kconfig options and select the necessary ones in your
application’s **`prj.conf`**.

Check:

- Dependencies.
- Default values.
- Options selected automatically.
- Whether additional modes or backends are required.

Do not edit Zephyr’s Kconfig definitions just to enable a feature.

See [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md)
for the full search workflow.

---

## 3. Describe the Hardware When Required

For hardware features, inspect the board’s devicetree and add an **overlay** when
necessary.

The hardware description may specify:

- Device instance.
- Enabled/disabled status.
- Pin connections and routing.
- Polarity.
- Bus settings or other device properties.

**Software-only modules may not require devicetree configuration.**

See [Devicetree Overlays Guide](<../04_Kconfig_vs_Devicetree(DTS)/Devicetree_Overlays_Guide.md>)
for how to find binding properties and write overlays.

---

## 4. Understand Initialization

### Hardware drivers

Zephyr normally initializes device-model drivers during startup when the required
driver and device instance are included in the build.

However:

- Initialization can fail.
- Some devices use deferred initialization.
- Driver initialization does not necessarily configure everything your
  application needs.

**Kconfig and devicetree are processed during the build. The firmware does not
read `prj.conf` or overlay files as text at startup.**

### Software modules

Do not assume every software module initializes automatically.

Read its API to determine whether you must:

- Initialize a module.
- Create or initialize an object.
- Register callbacks.
- Enable a service.
- Start an operation.

---

## 5. Obtain the Device or Object

Follow the pattern required by the API.

| Pattern | Example |
|---|---|
| Device pointer from devicetree | A UART or LED-strip device |
| Hardware specification from devicetree | `struct gpio_dt_spec` |
| Application-created object | A synchronization or work object |
| Module registration | A logging module |

**Obtaining a reference is not the same as initializing or configuring the
feature.**

For example, `GPIO_DT_SPEC_GET` extracts the GPIO controller, pin, and flags. It
does not configure the pin as an output.

---

## 6. Check Readiness

For device-model hardware, check that the device is ready before accessing it.

Examples:

- `device_is_ready` for a device pointer.
- `gpio_is_ready_dt` for a GPIO specification.

If the check fails, report the problem and avoid operating the device.

**Device readiness does not verify external wiring or guarantee that every
optional operation is supported.**

---

## 7. Configure the Feature for Your Application

Perform the configuration required by the API.

Examples:

| Feature | Application preparation |
|---|---|
| External GPIO LED | Configure the pin as an output with an initial state |
| GPIO button | Configure input mode and, if needed, interrupts and callbacks |
| UART | Select the desired API mode and perform any required configuration |
| Addressable RGB LED | Prepare pixel-colour data |
| Software module | Initialize/register objects or services as documented |

Only reconfigure when necessary, and coordinate changes if multiple parts of
your application share the resource.

---

## 8. Perform Operations and Handle Results

Call the public API operations your application needs.

Examples:

- Set or toggle a GPIO output.
- Read a GPIO input.
- Send or receive UART data.
- Update RGB pixels.
- Submit work or write log messages.

**Read each function’s return-value documentation.**

Do not assume every function uses the same convention:

- Some return `0` on success and a negative error on failure.
- Some return data, a count, or a Boolean.
- Some return nothing.
- Asynchronous operations may report completion through callbacks or events.

---

## 9. Build and Verify

Inspect:

| Generated file | What it shows |
|---|---|
| `build/zephyr/.config` | Final software configuration |
| `build/zephyr/zephyr.dts` | Final hardware description |

Keep permanent changes in your application files — not these generated outputs.

Test one feature at a time before combining them.

---

## Example Workflow: External GPIO LED

1. Enable GPIO support.
2. Describe the LED’s controller, pin, and polarity in the overlay.
3. Include the GPIO public header.
4. Create a `gpio_dt_spec`.
5. Check the GPIO controller’s readiness.
6. Configure the pin as an output, initially inactive.
7. Set or toggle the LED.
8. Check operation results.
9. Add a delay if blinking.

### `prj.conf`

```conf
CONFIG_GPIO=y
```

### `app.overlay`

```dts
#include <zephyr/dt-bindings/gpio/gpio.h>

/ {
    leds {
        compatible = "gpio-leds";

        external_led: led_7 {
            gpios = <&gpio0 7 GPIO_ACTIVE_HIGH>;
            label = "External LED on GPIO7";
        };
    };

    aliases {
        led0 = &external_led;
    };
};
```

### `src/main.c`

```c
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

int main(void)
{
    if (!gpio_is_ready_dt(&led)) {
        return -1;
    }

    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) < 0) {
        return -1;
    }

    while (1) {
        gpio_pin_toggle_dt(&led);
        k_msleep(500);
    }
}
```

The overlay only describes the hardware. The application still has to check
readiness, configure the pin, and call the operations.

---

## Final Checklist

- [ ] Find the feature’s public API and a relevant sample.
- [ ] Enable its required Kconfig options.
- [ ] Configure devicetree hardware if needed.
- [ ] Determine what initializes automatically.
- [ ] Obtain or create the required device/object.
- [ ] Check readiness where applicable.
- [ ] Perform application-specific initialization/configuration.
- [ ] Call public operations.
- [ ] Handle errors and completion events.
- [ ] Test and verify.

**Remember: driver initialized ≠ application configured ≠ operation completed
successfully.**

---

## Related documents

- [Create a New Zephyr Project](README.md) — full project examples.
- [Finding Features and Kconfig](../05_KConfig/Finding_Features_And_Kconfig.md)
  — how to find Kconfig options and drivers.
- [Devicetree Overlays Guide](<../04_Kconfig_vs_Devicetree(DTS)/Devicetree_Overlays_Guide.md>)
  — where to find overlay properties and how to write overlays.
- [Kconfig deep dive](../05_KConfig/README.md) — symbols, `depends on`,
  `select`, `default`, priority hierarchy.
