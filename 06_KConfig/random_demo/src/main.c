/*
 * Copyright (c) 2026 ITI Zephyr Playground
 * SPDX-License-Identifier: Apache-2.0
 *
 * Random-number demo driven by Kconfig.
 *
 * The two Kconfig options live in this app's Kconfig file:
 *   CONFIG_APP_RANDOM_MAX          -> upper bound of the random number
 *   CONFIG_APP_RANDOM_INTERVAL_MS  -> delay between numbers
 *
 * Change them in prj.conf (or with menuconfig), rebuild, and the behavior
 * changes without editing this C file.
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/printk.h>

#include <app_config.h>

int main(void)
{
	printk("%s started (max=%d, interval=%d ms)\n",
	       APP_BANNER,
	       CONFIG_APP_RANDOM_MAX,
	       CONFIG_APP_RANDOM_INTERVAL_MS);

	while (1) {
		uint32_t value = sys_rand32_get() % CONFIG_APP_RANDOM_MAX;

		printk("Random number: %u\n", value);
		k_msleep(CONFIG_APP_RANDOM_INTERVAL_MS);
	}

	return 0;
}
