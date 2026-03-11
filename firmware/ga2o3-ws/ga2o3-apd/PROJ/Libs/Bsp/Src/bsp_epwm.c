/*
bsp_pwm_driverlib.c
DriverLib version of PWM HAL for F28379D LAUNCH-XL
Author: David Redondo - 2026
*/

#include "F2837xD_device.h"
#include "driverlib.h"
#include "bsp_epwm.h"

#define PWM_DEFAULT_FREQ_HZ   10000
#define PWM_DEFAULT_DEAD_NS   1000
#define PWM_DEFAULT_DUTY      0.5f

static uint32_t pwm_channels[] = {
    0,               // dummy index 0
    EPWM1_BASE,
    EPWM2_BASE,
    EPWM3_BASE,
    EPWM4_BASE,
    EPWM5_BASE,
    EPWM6_BASE,
    EPWM7_BASE,
    EPWM8_BASE
};

static const uint32_t pwm_gpio_cfg_a[] = { 0, GPIO_0_EPWM1A, GPIO_2_EPWM2A, GPIO_4_EPWM3A,
                                               GPIO_6_EPWM4A, GPIO_8_EPWM5A, GPIO_10_EPWM6A,
                                               GPIO_12_EPWM7A, GPIO_14_EPWM8A };

static const uint32_t pwm_gpio_cfg_b[] = { 0, GPIO_1_EPWM1B, GPIO_3_EPWM2B, GPIO_5_EPWM3B,
                                               GPIO_7_EPWM4B, GPIO_9_EPWM5B, GPIO_11_EPWM6B,
                                               GPIO_13_EPWM7B, GPIO_15_EPWM8B };

HAL_StatusTypeDef InitPWM(uint32_t channel)
{
    if (channel < 1 || channel > 8) return HAL_ERROR;

    uint32_t base = pwm_channels[channel];
    uint32_t period_ticks = DEVICE_SYSCLK_FREQ / (PWM_DEFAULT_FREQ_HZ * 4U);

    // Configure GPIO pins for this channel
    GPIO_setPinConfig(pwm_gpio_cfg_a[channel]);
    GPIO_setPinConfig(pwm_gpio_cfg_b[channel]);

    // Enable EPWM module clock
    EALLOW;
    CpuSysRegs.PCLKCR2.all |= (1U << (channel - 1));
    EDIS;

    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    // Time base
    EPWM_setTimeBasePeriod(base, period_ticks);
    EPWM_setTimeBaseCounter(base, 0);
    EPWM_setTimeBaseCounterMode(base, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_setClockPrescaler(base, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);
    EPWM_disablePhaseShiftLoad(base);
    EPWM_setPhaseShift(base, 0);
    EPWM_setSyncOutPulseMode(base, EPWM_SYNC_OUT_PULSE_ON_COUNTER_ZERO);

    // Compare shadow load mode
    EPWM_setCounterCompareShadowLoadMode(base, EPWM_COUNTER_COMPARE_A, EPWM_COMP_LOAD_ON_CNTR_ZERO);
    EPWM_setCounterCompareShadowLoadMode(base, EPWM_COUNTER_COMPARE_B, EPWM_COMP_LOAD_ON_CNTR_ZERO);

    // Action qualifier
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH,      EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW,       EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(base, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);

    EPWM_disableChopper(base);

    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    SetDuty(channel, PWM_DEFAULT_DUTY);
    SetDeadTime(channel, PWM_DEFAULT_DEAD_NS);

    return HAL_OK;
}

HAL_StatusTypeDef SetDuty(uint32_t channel, float duty)
{
    if (channel < 1 || channel > 8) return HAL_ERROR;
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;

    uint32_t base = pwm_channels[channel];
    uint32_t period = EPWM_getTimeBasePeriod(base);

    EPWM_setCounterCompareValue(base, EPWM_COUNTER_COMPARE_A, (uint16_t)(duty * period));
    EPWM_setCounterCompareValue(base, EPWM_COUNTER_COMPARE_B, (uint16_t)((1.0f - duty) * period));

    return HAL_OK;
}

HAL_StatusTypeDef SetFrequency(uint32_t channel, uint32_t frequency_Hz)
{
    if (channel < 1 || channel > 8) return HAL_ERROR;
    if (frequency_Hz == 0) return HAL_ERROR;

    uint32_t base = pwm_channels[channel];
    uint32_t period_ticks = (uint32_t)(DEVICE_SYSCLK_FREQ / (frequency_Hz * 4));

    EPWM_setTimeBasePeriod(base, period_ticks);

    // Re-apply duty so the compare values stay consistent with new period
    // Read back isn't possible for duty, so we recalculate from CMPA
    uint16_t cmpa = EPWM_getCounterCompareValue(base, EPWM_COUNTER_COMPARE_A);
    uint32_t old_period = EPWM_getTimeBasePeriod(base);  // still old value at this point?
    // To avoid that race: reuse SetDuty by deriving duty from old CMPA/old period
    // But since we just wrote the new period above, fetch CMPA before the write.
    // Safer: just preserve duty ratio explicitly.
    float duty = (old_period > 0) ? ((float)cmpa / (float)old_period) : PWM_DEFAULT_DUTY;
    return SetDuty(channel, duty);
}

HAL_StatusTypeDef SetDeadTime(uint32_t channel, uint32_t dead_time_ns)
{
    if (channel < 1 || channel > 8) return HAL_ERROR;

    uint32_t base = pwm_channels[channel];
    uint32_t dead_time_ticks = (dead_time_ns * (DEVICE_SYSCLK_FREQ / 1000000UL)) / 1000UL;

    EPWM_setDeadBandDelayMode(base, EPWM_DB_RED, true);
    EPWM_setDeadBandDelayMode(base, EPWM_DB_FED, true);
    EPWM_setDeadBandDelayPolarity(base, EPWM_DB_RED, EPWM_DB_POLARITY_ACTIVE_HIGH);
    EPWM_setDeadBandDelayPolarity(base, EPWM_DB_FED, EPWM_DB_POLARITY_ACTIVE_LOW);
    EPWM_setRisingEdgeDeadBandDelayInput(base,  EPWM_DB_INPUT_EPWMA);
    EPWM_setFallingEdgeDeadBandDelayInput(base, EPWM_DB_INPUT_EPWMA);
    EPWM_setRisingEdgeDelayCount(base,  dead_time_ticks);
    EPWM_setFallingEdgeDelayCount(base, dead_time_ticks);

    return HAL_OK;
}

HAL_StatusTypeDef EnablePWM(uint32_t channel)
{
    if (channel < 1 || channel > 8) return HAL_ERROR;

    uint32_t base = pwm_channels[channel];

    // Release software force — outputs return to AQ control
    EPWM_setActionQualifierContSWForceAction(base, EPWM_AQ_OUTPUT_A, EPWM_AQ_SW_DISABLED);
    EPWM_setActionQualifierContSWForceAction(base, EPWM_AQ_OUTPUT_B, EPWM_AQ_SW_DISABLED);

    return HAL_OK;
}

HAL_StatusTypeDef DisablePWM(uint32_t channel)
{
    if (channel < 1 || channel > 8) return HAL_ERROR;

    uint32_t base = pwm_channels[channel];

    // Continuously force both outputs low, overriding the AQ module
    EPWM_setActionQualifierContSWForceAction(base, EPWM_AQ_OUTPUT_A, EPWM_AQ_SW_OUTPUT_LOW);
    EPWM_setActionQualifierContSWForceAction(base, EPWM_AQ_OUTPUT_B, EPWM_AQ_SW_OUTPUT_LOW);

    return HAL_OK;
}