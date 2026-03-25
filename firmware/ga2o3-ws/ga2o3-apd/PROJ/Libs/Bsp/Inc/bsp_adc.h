/*
 * bsp_adc.h
 * ADC HAL for F28379D LAUNCHXL - DriverLib backend
 * Author: David Redondo - 2026
 *
 * Modules are 1-indexed: 1 = ADCA, 2 = ADCB, 3 = ADCC, 4 = ADCD.
 * SOC numbers are 0-indexed (0..15), matching DriverLib's ADC_SOCNumber enum.
 * channel / trigger / int_num use DriverLib's ADC_Channel / ADC_Trigger /
 * ADC_IntNumber enums directl - no re-wrapping.
 */

#ifndef BSP_ADC_H
#define BSP_ADC_H

#include "F2837xD_device.h"
#include "driverlib.h"
#include "bsp_epwm.h"   /* pulls in HAL_StatusTypeDef */

/* -----------------------------------------------------------------------
 * Core API
 * ----------------------------------------------------------------------- */

/*
 * InitADC: enable clock, set prescaler (ADCCLK = SYSCLK/4), configure
 * 12-bit single-ended mode, then release the converter and wait the
 * mandatory 1 ms power-up time.  Call once per module before any SOC work.
 */
HAL_StatusTypeDef InitADC(uint32_t module);

/*
 * ConfigureSOC: bind an input channel, hardware trigger, and sample
 * window to one SOC.  sample_window_ns is converted to SYSCLK cycles and
 * clamped to the 12-bit minimum (15 cycles / 75 ns at 200 MHz).
 *
 * channel  : ADC_CH_ADCIN0 .. ADC_CH_ADCIN15  (DriverLib ADC_Channel)
 * trigger  : ADC_TRIGGER_SW_ONLY, ADC_TRIGGER_EPWM1_SOCA, …  (ADC_Trigger)
 */
HAL_StatusTypeDef ConfigureSOC(uint32_t module, uint32_t soc,
                                ADC_Channel channel, ADC_Trigger trigger,
                                uint32_t sample_window_ns);

/*
 * ConfigureADCInterrupt: route an ADCxINTn interrupt to the end-of-
 * conversion of the given SOC, clear any stale flag, and arm the interrupt.
 * Does NOT register a PIE IS -that stays in the application layer.
 *
 * int_num : ADC_INT_NUMBER1 .. ADC_INT_NUMBER4
 */
HAL_StatusTypeDef ConfigureADCInterrupt(uint32_t module, ADC_IntNumber int_num,
                                         uint32_t soc);

/* -----------------------------------------------------------------------
 * Runtime API
 * ----------------------------------------------------------------------- */

/*
 * SoftwareTriggerSOC: assert a one-shot software force on the SOC.
 * Only meaningful when the SOC's trigger source is ADC_TRIGGER_SW_ONLY.
 */
HAL_StatusTypeDef SoftwareTriggerSOC(uint32_t module, uint32_t soc);

/*
 * GetADCInterruptFlag: returns true if the ADCxINTn flag is set.
 * Returns false on an invalid module (fail-safe, no HAL_ERROR here so the
 * caller can use it directly in a while-loop without an out-parameter).
 */
bool GetADCInterruptFlag(uint32_t module, ADC_IntNumber int_num);

/*
 * ClearADCInterruptFlag: clear the ADCxINTn status bit.  Call after
 * reading the result in polled mode, or at the start of the ISR.
 * Also clears the overflow flag if se -overflow means the previous result
 * was not read before the next conversion completed.
 */
HAL_StatusTypeDef ClearADCInterruptFlag(uint32_t module, ADC_IntNumber int_num);

/*
 * GetResult: read the 12-bit conversion result for a SOC from the
 * result register file.  Returns 0 on an invalid module/SO -callers
 * that need to distinguish a genuine 0 from an error should validate
 * arguments themselves before calling.
 */
uint16_t GetADCResult(uint32_t module, uint32_t soc);

#endif /* BSP_ADC_H */
