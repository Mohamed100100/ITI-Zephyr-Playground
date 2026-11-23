#ifndef POWER_SWITCH_H
#define POWER_SWITCH_H

#include <zephyr/drivers/gpio.h>

struct power_switch {
	struct gpio_dt_spec gpio;
};

int power_switch_init(struct power_switch *sw, const struct gpio_dt_spec *spec);
int power_switch_set(struct power_switch *sw, int value);

#endif
