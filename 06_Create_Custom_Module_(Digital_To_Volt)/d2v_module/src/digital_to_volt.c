/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Digital_To_Volt — implementation.
 * Pure integer math for the millivolt path; float only in the volt helpers.
 */

#include <digital_to_volt/digital_to_volt.h>

uint32_t d2v_max_raw(uint8_t resolution_bits)
{
	if (resolution_bits == 0U || resolution_bits > 31U) {
		return 0U;
	}

	return (resolution_bits >= 32U) ? 0xFFFFFFFFU : ((1UL << resolution_bits) - 1UL);
}

int32_t d2v_raw_to_mv(uint32_t raw, uint8_t resolution_bits, uint32_t vref_mv)
{
	uint32_t max_raw;
	uint64_t scaled;

	if (resolution_bits == 0U || resolution_bits > 31U) {
		return 0;
	}

	max_raw = d2v_max_raw(resolution_bits);
	if (max_raw == 0U) {
		return 0;
	}

	/* Clamp over-range codes (can happen with oversampling / noise). */
	if (raw > max_raw) {
		raw = max_raw;
	}

	/* mv = raw * vref / max_raw — 64-bit so nothing overflows. */
	scaled = (uint64_t)raw * (uint64_t)vref_mv;
	scaled /= max_raw;

	return (int32_t)scaled;
}

float d2v_mv_to_volt(int32_t mv)
{
	return ((float)mv) / 1000.0f;
}

float d2v_raw_to_volt(uint32_t raw, uint8_t resolution_bits, uint32_t vref_mv)
{
	return d2v_mv_to_volt(d2v_raw_to_mv(raw, resolution_bits, vref_mv));
}

bool d2v_is_above_threshold_mv(int32_t mv, int32_t threshold_mv)
{
	return mv > threshold_mv;
}
