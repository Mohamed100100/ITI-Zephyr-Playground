# SPDX-License-Identifier: Apache-2.0
# Digital_To_Volt out-of-tree module — build glue.
# Zephyr includes this file because zephyr/module.yml points at it.

if(CONFIG_DIGITAL_TO_VOLT)

    zephyr_library()


    zephyr_library_sources(
        src/digital_to_volt.c
    )


    zephyr_include_directories(
        include
    )


endif()
