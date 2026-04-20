/*
 * bsp_adc.c
 * ADC HAL for F28379D LAUNCHXL  -  DriverLib backend
 * Author: David Redondo - 2026
 */

#include "F2837xD_device.h"
#include "driverlib.h"
#include "bsp_adc.h"

/* -----------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------- */

/*
 * ADCCLK = SYSCLK / 4  ->  50 MHz at 200 MHz SYSCLK.
 * The ADC's absolute maximum input clock is 50 MHz (Table 5-27 in the
 * datasheet), so DIV_4 is the tightest legal prescaler at full CPU speed.
 */
#define ADC_PRESCALER           ADC_CLK_DIV_4_0

/*
 * In 12-bit mode the ACQPS field encodes (sample_window - 1) cycles, and
 * the minimum legal ACQPS value is 14, giving a 15-cycle window.
 * DriverLib's ADC_setupSOC() applies the -1 internally, so we pass 15.
 */
#define ADC_MIN_SAMPLE_CYCLES   15U

/* -----------------------------------------------------------------------
 * Lookup tables  (index 0 is a dummy to allow 1-based module numbers)
 * ----------------------------------------------------------------------- */

static const uint32_t adc_base[] = {
    0,
    ADCA_BASE,
    ADCB_BASE,
    ADCC_BASE,
    ADCD_BASE
};

static const uint32_t adc_result_base[] = {
    0,
    ADCARESULT_BASE,
    ADCBRESULT_BASE,
    ADCCRESULT_BASE,
    ADCDRESULT_BASE
};

static const SysCtl_PeripheralPCLOCKCR adc_periph[] = {
    (SysCtl_PeripheralPCLOCKCR)0,
    SYSCTL_PERIPH_CLK_ADCA,
    SYSCTL_PERIPH_CLK_ADCB,
    SYSCTL_PERIPH_CLK_ADCC,
    SYSCTL_PERIPH_CLK_ADCD
};

/* -----------------------------------------------------------------------
 * Private helpers
 * ----------------------------------------------------------------------- */

static inline bool module_valid(uint32_t module) { return (module >= 1U) && (module <= 4U); }
static inline bool soc_valid(uint32_t soc)       { return (soc <= 15U); }

/* Convert a nanosecond sample window to SYSCLK cycles, then clamp. */
static inline uint32_t ns_to_cycles_clamped(uint32_t ns)
{
    uint32_t cycles = (ns * (DEVICE_SYSCLK_FREQ / 1000000UL)) / 1000UL;
    return (cycles < ADC_MIN_SAMPLE_CYCLES) ? ADC_MIN_SAMPLE_CYCLES : cycles;
}

/* -----------------------------------------------------------------------
 * Core API
 * ----------------------------------------------------------------------- */

HAL_StatusTypeDef InitADC(uint32_t module)
{
    if (!module_valid(module)) return HAL_ERROR;

    uint32_t base = adc_base[module];

    SysCtl_enablePeripheral(adc_periph[module]);

    ADC_setPrescaler(base, ADC_PRESCALER);

    ADC_setMode(base, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);

    /* INT pulse fires at end-of-conversion so the result is ready by the
     * time the ISR or polling loop sees the flag. */
    ADC_setInterruptPulseMode(base, ADC_PULSE_END_OF_CONV);

    ADC_enableConverter(base);

    /* Datasheet (Section 16.3.1) requires at least 1 ms after enabling the
     * converter before the first conversion is started. */
    DEVICE_DELAY_US(1000);

    return HAL_OK;
}

HAL_StatusTypeDef ConfigureSOC(uint32_t module, uint32_t soc,
                                ADC_Channel channel, ADC_Trigger trigger,
                                uint32_t sample_window_ns)
{
    if (!module_valid(module)) return HAL_ERROR;
    if (!soc_valid(soc))       return HAL_ERROR;

    uint32_t cycles = ns_to_cycles_clamped(sample_window_ns);

    ADC_setupSOC(adc_base[module], (ADC_SOCNumber)soc, trigger, channel, cycles);

    return HAL_OK;
}

HAL_StatusTypeDef ConfigureADCInterrupt(uint32_t module, ADC_IntNumber int_num,
                                         uint32_t soc)
{
    if (!module_valid(module)) return HAL_ERROR;
    if (!soc_valid(soc))       return HAL_ERROR;

    uint32_t base = adc_base[module];

    ADC_setInterruptSource(base, int_num, (ADC_SOCNumber)soc);

    /* Clear before arming - if a conversion already completed the flag
     * would fire the ISR immediately on the first enable otherwise. */
    ADC_clearInterruptStatus(base, int_num);

    ADC_enableInterrupt(base, int_num);

    return HAL_OK;
}

/* -----------------------------------------------------------------------
 * Runtime API
 * ----------------------------------------------------------------------- */

HAL_StatusTypeDef SoftwareTriggerSOC(uint32_t module, uint32_t soc)
{
    if (!module_valid(module)) return HAL_ERROR;
    if (!soc_valid(soc))       return HAL_ERROR;

    ADC_forceSOC(adc_base[module], (ADC_SOCNumber)soc);

    return HAL_OK;
}

bool GetADCInterruptFlag(uint32_t module, ADC_IntNumber int_num)
{
    if (!module_valid(module)) return false;

    return ADC_getInterruptStatus(adc_base[module], int_num);
}

HAL_StatusTypeDef ClearADCInterruptFlag(uint32_t module, ADC_IntNumber int_num)
{
    if (!module_valid(module)) return HAL_ERROR;

    uint32_t base = adc_base[module];

    /* The overflow flag latches independently of the status flag.
     * If we only clear the status flag and leave overflow set, the
     * next conversion won't retrigger the interrupt on some silicon
     * revisions.  Clear both unconditionally. */
    if (ADC_getInterruptOverflowStatus(base, int_num))
        ADC_clearInterruptOverflowStatus(base, int_num);

    ADC_clearInterruptStatus(base, int_num);

    return HAL_OK;
}

uint16_t GetADCResult(uint32_t module, uint32_t soc)
{
    if (!module_valid(module)) return 0U;
    if (!soc_valid(soc))       return 0U;

    return ADC_readResult(adc_result_base[module], (ADC_SOCNumber)soc);
}
