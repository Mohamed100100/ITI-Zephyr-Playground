/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Digital_To_Volt demo app.
 *
 * Two modes, selected automatically at BUILD time from Devicetree:
 *   io-channels overlay present (Black Pill + boards/*.overlay) -> reads the
 *                real ADC pin (PA1) with the Zephyr ADC API, converts with
 *                digital_to_volt(), prints volts, drives LED0 by threshold.
 *   no io-channels (native_sim / QEMU) -> sweeps raw 0..4095 in software so the
 *                conversion + LED logic can be tested with no hardware at all.
 *
 * LED threshold = half scale (VREF / 2).
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <digital_to_volt/digital_to_volt.h>

#if !defined(CONFIG_DIGITAL_TO_VOLT) || (CONFIG_DIGITAL_TO_VOLT == 0)
#error "CONFIG_DIGITAL_TO_VOLT must be enabled for this demo"
#endif

LOG_MODULE_REGISTER(d2v_demo, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Board does not define a usable 'led0' alias"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* LED threshold = half of the reference voltage. */
#define LED_THRESHOLD_V (CONFIG_DIGITAL_TO_VOLT_VREF_MV / 2000.0f)

/* Real ADC path is compiled only when an io-channels overlay exists.
 * Otherwise the simulate path below is used. */
#if DT_NODE_EXISTS(DT_PATH(zephyr_user)) && \
	DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#define D2V_USE_REAL_ADC 1
#include <zephyr/drivers/adc.h>

static const struct adc_dt_spec adc_chan =
	ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
#else
#define D2V_USE_REAL_ADC 0
#endif

static void drive_led(float volts)
{
	gpio_pin_set_dt(&led, volts > LED_THRESHOLD_V ? 1 : 0);
}

#if D2V_USE_REAL_ADC
static int read_adc_raw(uint32_t *raw_out)
{
	int err;
	int16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
	};

	err = adc_sequence_init_dt(&adc_chan, &sequence);
	if (err < 0) {
		LOG_ERR("adc_sequence_init_dt failed (%d)", err);
		return err;
	}

	err = adc_read_dt(&adc_chan, &sequence);
	if (err < 0) {
		LOG_ERR("adc_read failed (%d)", err);
		return err;
	}

	*raw_out = (uint32_t)buf;
	return 0;
}
#endif /* D2V_USE_REAL_ADC */

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED device not ready");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("LED configure failed (%d)", ret);
		return 0;
	}

#if D2V_USE_REAL_ADC
	if (!adc_is_ready_dt(&adc_chan)) {
		LOG_ERR("ADC device not ready");
		return 0;
	}

	ret = adc_channel_setup_dt(&adc_chan);
	if (ret < 0) {
		LOG_ERR("ADC channel setup failed (%d)", ret);
		return 0;
	}

	LOG_INF("D2V demo: real ADC mode");

	while (1) {
		uint32_t raw;
		float volts;

		if (read_adc_raw(&raw) == 0) {
			volts = digital_to_volt(raw);
			LOG_INF("raw=%u -> %.3f V", raw, (double)volts);
			drive_led(volts);
		}
		k_msleep(500);
	}
#else
	uint32_t raw = 0;

	LOG_INF("D2V demo: SIMULATE mode");

	while (1) {
		float volts = digital_to_volt(raw);

		LOG_INF("raw=%u -> %.3f V", raw, (double)volts);
		drive_led(volts);

		raw += 410;
		if (raw > 4095) {
			raw = 0;
		}
		k_msleep(500);
	}
#endif
}
