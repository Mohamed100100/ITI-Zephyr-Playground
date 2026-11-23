#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <app_config.h>

/* The WeAct ESP32-S3-Mini exposes its on-board WS2812 RGB LED
 * through the "rgb0" devicetree alias.
 */
#define STRIP_NODE DT_ALIAS(rgb0)

#if !DT_NODE_EXISTS(STRIP_NODE)
#error "Board does not define an 'rgb0' alias"
#endif

#define STRIP_LEN DT_PROP(STRIP_NODE, chain_length)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[STRIP_LEN];

static const struct led_rgb colors[] = {
	RGB(0x20, 0x00, 0x00), /* red */
	RGB(0x00, 0x20, 0x00), /* green */
	RGB(0x00, 0x00, 0x20), /* blue */
	RGB(0x00, 0x00, 0x00), /* off */
};

int main(void)
{
	if (!device_is_ready(strip)) {
		printk("RGB LED strip device is not ready\n");
		return 0;
	}

	printk("ESP32-S3 Mini blinky started\n");

	while (1) {
		for (size_t color = 0; color < ARRAY_SIZE(colors); color++) {
			for (size_t i = 0; i < STRIP_LEN; i++) {
				pixels[i] = colors[color];
			}

			int ret = led_strip_update_rgb(strip, pixels, STRIP_LEN);

			if (ret < 0) {
				printk("led_strip_update_rgb failed: %d\n", ret);
				return 0;
			}

			k_msleep(APP_COLOR_DELAY_MS);
		}
	}

	return 0;
}
