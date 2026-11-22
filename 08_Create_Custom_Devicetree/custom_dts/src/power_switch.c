#include <power_switch/power_switch.h>

#include <errno.h>
#include <zephyr/kernel.h>

int power_switch_init(struct power_switch *sw, const struct gpio_dt_spec *spec)
{
	int err;

	if (sw == NULL || spec == NULL) {
		return -EINVAL;
	}

	sw->gpio = *spec;

	if (!gpio_is_ready_dt(&sw->gpio)) {
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&sw->gpio, GPIO_OUTPUT_INACTIVE);
	return err;
}

int power_switch_set(struct power_switch *sw, int value)
{
	if (sw == NULL) {
		return -EINVAL;
	}
	return gpio_pin_set_dt(&sw->gpio, value);
}
