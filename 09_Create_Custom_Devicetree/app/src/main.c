/*
 * Copyright (c) 2026 ITI Zephyr Playground
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include <power_switch/power_switch.h>

#include <app_config.h>

#if !DT_NODE_EXISTS(DT_NODELABEL(load_switch))
#error "Overlay for the power-switch node is not defined."
#endif

static const struct gpio_dt_spec load_switch_spec =
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(load_switch), gpios, {0});

static struct power_switch load_switch;

int main(void)
{
	int err;

	err = power_switch_init(&load_switch, &load_switch_spec);
	if (err != 0) {
		printf("Initializing power switch failed: %d\n", err);
		return 0;
	}

	printf("Waiting one second.\n");
	k_sleep(K_MSEC(APP_START_DELAY_MS));

	printf("Setting load switch to active level.\n");
	err = power_switch_set(&load_switch, 1);
	if (err != 0) {
		printf("Setting load switch level failed: %d\n", err);
	}
	return 0;
}
