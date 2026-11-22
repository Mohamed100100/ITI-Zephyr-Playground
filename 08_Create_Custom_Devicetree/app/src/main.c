/*
 * Copyright (c) 2026 ITI Zephyr Playground
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#if !DT_NODE_EXISTS(DT_NODELABEL(load_switch))
#error "Overlay for the power-switch node is not defined."
#endif

static const struct gpio_dt_spec load_switch =
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(load_switch), gpios, {0});

int main(void)
{
	int err;

	if (!gpio_is_ready_dt(&load_switch)) {
		printf("The load-switch GPIO port is not ready.\n");
		return 0;
	}

	printf("Initializing load switch (inactive).\n");

	err = gpio_pin_configure_dt(&load_switch, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		printf("Configuring GPIO pin failed: %d\n", err);
		return 0;
	}

	printf("Waiting one second.\n");
	k_sleep(K_MSEC(1000));

	printf("Setting load switch to active level.\n");
	err = gpio_pin_set_dt(&load_switch, 1);
	if (err != 0) {
		printf("Setting GPIO level failed: %d\n", err);
	}
	return 0;
}
