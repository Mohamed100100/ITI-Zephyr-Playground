#include <digital_to_volt/digital_to_volt.h>

/* NOTE: CONFIG_DIGITAL_TO_VOLT_VREF_MV and CONFIG_DIGITAL_TO_VOLT_RESOLUTION_BITS
 * come from the generated autoconf.h. You never need to include it:
 * zephyr/CMakeLists.txt:376-378 passes it to every C/C++/ASM file via the
 * compiler's -imacros flag — i.e. it's force-included before your code,
 * includes or not. */


float digital_to_volt(uint32_t digital_value)
{
    uint32_t max_value =
        (1UL << CONFIG_DIGITAL_TO_VOLT_RESOLUTION_BITS) - 1UL;

    return ((float)digital_value *
            CONFIG_DIGITAL_TO_VOLT_VREF_MV) /
           max_value;
}
