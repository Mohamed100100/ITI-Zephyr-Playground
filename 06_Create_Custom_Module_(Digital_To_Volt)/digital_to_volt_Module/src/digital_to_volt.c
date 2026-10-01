#include <digital_to_volt/digital_to_volt.h>
#include <zephyr/sys/util.h>


float digital_to_volt(uint32_t digital_value)
{
    return ((float)digital_value *
            CONFIG_DIGITAL_TO_VOLT_VREF_MV) /
           CONFIG_DIGITAL_TO_VOLT_RESOLUTION_BITS;
}
