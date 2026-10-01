# SPDX-License-Identifier: Apache-2.0
# Digital_To_Volt out-of-tree module — build glue.
# Zephyr includes this file because zephyr/module.yml points at it.

zephyr_library_named(digital_to_volt)

# Core conversion code: compiled only when the feature is enabled.
zephyr_library_sources_ifdef(CONFIG_DIGITAL_TO_VOLT src/digital_to_volt.c)

# Public headers: app code does #include <digital_to_volt/digital_to_volt.h>
zephyr_include_directories(include)
