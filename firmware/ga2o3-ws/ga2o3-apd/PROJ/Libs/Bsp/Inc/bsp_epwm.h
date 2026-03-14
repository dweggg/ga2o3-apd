/*

bsp_pwm.h

This file is the header of a PWM HAL for a F28379D LAUNCH-XL.

It exposes a small API to init channels and set frequency, duty
and dead time to each EPWM channel separately.

David Redondo - 2026

*/

#ifndef __BSP_PWM_H__
#define __BSP_PWM_H__


#include "bsp_hal.h"
#include <stdint.h>

/**
 * @brief Initializes a PWM channel with frequency = 10kHz, deadtime = 1000ns and duty = 0.
 * @param[in] channel The generic channel to initialize (1-8).
 * @return HAL_OK if successful, HAL_ERROR if any argument is invalid.
*/
HAL_StatusTypeDef InitPWM(uint32_t channel);

/**
 * @brief Enables a PWM channel.
 * @param[in] channel The generic channel to enable (1-8).
 * @return HAL_OK if successful, HAL_ERROR if any argument is invalid.
*/
HAL_StatusTypeDef EnablePWM(uint32_t channel);

/**
 * @brief Disables a PWM channel.
 * @param[in] channel The generic channel to disable (1-8).
 * @return HAL_OK if successful, HAL_ERROR if any argument is invalid.
*/
HAL_StatusTypeDef DisablePWM(uint32_t channel);

/**
 * @brief Sets a new deadtime for a specific PWM channel.
 * @param[in] channel The channel to apply the dead time to (1-8).
 * @param[in] dead_time The desired dead time in nanoseconds.
 * @return HAL_OK if successful, HAL_ERROR if any argument is invalid.
*/
HAL_StatusTypeDef SetDeadTime(uint32_t channel, uint32_t dead_time);

/**
 * @brief Sets a new frequency for a specific PWM channel.
 * @param[in] channel The generic channel to apply the frequency to (1-8).
 * @param[in] frequency The desired PWM frequency in Hz.
 * @return HAL_OK if successful, HAL_ERROR if any argument is invalid.
*/
HAL_StatusTypeDef SetFrequency(uint32_t channel, uint32_t frequency);

/**
 * @brief Sets a new duty cycle for a specific PWM channel.
 * @param[in] channel The generic channel to apply the duty cycle to (1-8).
 * @param[in] duty The desired duty cycle in p.u (0..1).
 * @return HAL_OK if successful, HAL_ERROR if any argument is invalid.
*/
HAL_StatusTypeDef SetDuty(uint32_t channel, float duty);

/**
 * @brief Shifts the carrier of channel_2 relative to channel_1 by phase_shift_pu (0.0 = 0deg, 1.0 = 360deg)
 * @attention Make sure to use this function to force two channels to 0 phase shift if that's what you want. Consecutive channels start up with a small phase shift of ~20deg.
 * @param[in] channel_1 The master channel (1-8).
 * @param[in] channel_2 The slave channel (1-8).
 * @param[in] phase_shift_pu The phase shift from 0 to 1.
 * @return HAL_OK if successful, HAL_ERROR if any argument is invalid or if the channels have different periods.
 */
HAL_StatusTypeDef SetPhaseShift(uint32_t channel_1, uint32_t channel_2, float phase_shift_pu);

#endif
