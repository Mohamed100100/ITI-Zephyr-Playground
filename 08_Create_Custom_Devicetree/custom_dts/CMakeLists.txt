if(CONFIG_POWER_SWITCH)

    zephyr_library()

    zephyr_library_sources(
        src/power_switch.c
    )

    zephyr_include_directories(
        include
    )

endif()
