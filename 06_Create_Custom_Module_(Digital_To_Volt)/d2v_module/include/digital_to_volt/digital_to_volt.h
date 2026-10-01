/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Digital_To_Volt — public API.
 *
 * Converts raw ADC counts to millivolts / volts using integer math
 * (no float needed for the millivolt path).
 */
#ifndef DIGITAL_TO_VOLT_H_
#define DIGITAL_TO_VOLT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert a raw ADC reading to millivolts.
 *
 * Formula: mv = raw * vref_mv / (2^resolution_bits - 1)
 *
 * Uses 64-bit intermediate math so 16-bit raw * 5000 mV never overflows.
 *
 * @param raw             Raw ADC counts (0 .. 2^resolution_bits - 1).
 * @param resolution_bits ADC resolution, e.g. 12 for 0..4095.
 * @param vref_mv         Reference voltage in millivolts, e.g. 3300.
 *
 * @return Converted value in millivolts, clamped to [0, vref_mv].
 *         Returns 0 if resolution_bits is 0 or > 31.
 */
int32_t d2v_raw_to_mv(uint32_t raw, uint8_t resolution_bits, uint32_t vref_mv);

/**
 * @brief Convert millivolts to volts (float helper for printing).
 *
 * @param mv Millivolts.
 * @return Volts as float, e.g. 1650 -> 1.65f.
 */
float d2v_mv_to_volt(int32_t mv);

/**
 * @brief Convert raw ADC counts directly to volts.
 *
 * @param raw             Raw ADC counts.
 * @param resolution_bits ADC resolution bits.
 * @param vref_mv         Reference voltage in millivolts.
 *
 * @return Volts as float.
 */
float d2v_raw_to_volt(uint32_t raw, uint8_t resolution_bits, uint32_t vref_mv);

/**
 * @brief Check whether a millivolt reading is above a threshold.
 *
 * @param mv           Measured millivolts.
 * @param threshold_mv Threshold in millivolts.
 *
 * @return true if mv > threshold_mv, false otherwise.
 */
bool d2v_is_above_threshold_mv(int32_t mv, int32_t threshold_mv);

/**
 * @brief Maximum raw code for a given resolution.
 *
 * @param resolution_bits e.g. 12 -> 4095.
 * @return (2^resolution_bits - 1), or 0 for invalid input.
 */
uint32_t d2v_max_raw(uint8_t resolution_bits);

#ifdef __cplusplus
}
#endif

#endif /* DIGITAL_TO_VOLT_H_ */
