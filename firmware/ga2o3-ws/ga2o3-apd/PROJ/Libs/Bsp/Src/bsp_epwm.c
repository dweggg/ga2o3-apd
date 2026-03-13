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

static const uint32_t pwm_channels[] = {
    0, // dummy index 0
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
    uint32_t period_ticks = DEVICE_SYSCLK_FREQ / (PWM_DEFAULT_FREQ_HZ * 2U * 2U); // one of the *2 is due to the counter being up and down. I don't know why the other is there but if it's not the real frequency halves lol

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
    uint32_t new_period_ticks = (uint32_t)(DEVICE_SYSCLK_FREQ / (frequency_Hz * 2U * 2U)); // one of the *2 is due to the counter being up and down. I don't know why the other is there but if it's not the real frequency halves lol

    // Re-apply duty so the compare values stay consistent with new period
    uint16_t cmpa = EPWM_getCounterCompareValue(base, EPWM_COUNTER_COMPARE_A);
    uint32_t old_period = EPWM_getTimeBasePeriod(base);  // still old value at this point
    float duty = (old_period > 0) ? ((float)cmpa / (float)old_period) : PWM_DEFAULT_DUTY;

    // Update frequency now, after calculating the duty
    EPWM_setTimeBasePeriod(base, new_period_ticks);
    
    // Update CMPA with old duty before leaving
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

HAL_StatusTypeDef SetPhaseShift(uint32_t channel_1, uint32_t channel_2, float phase_shift_pu)
{
    // Validation
    if (channel_1 < 1 || channel_1 > 8) return HAL_ERROR;
    if (channel_2 < 1 || channel_2 > 8) return HAL_ERROR;
    if (channel_1 == channel_2)          return HAL_ERROR;

    if (phase_shift_pu < 0.0f) phase_shift_pu = 0.0f;
    if (phase_shift_pu > 1.0f) phase_shift_pu = 1.0f;

    // Enforce sync-chain direction (low > high)
    uint32_t master_ch, slave_ch;
    float shift;

    if (channel_1 < channel_2) {
        master_ch = channel_1;
        slave_ch  = channel_2;
        shift     = phase_shift_pu;
    } else {
        // Swap and invert: "delay ch2 by X" == "delay ch1 by 1−X" from ch2's view
        master_ch = channel_2;
        slave_ch  = channel_1;
        shift     = (phase_shift_pu == 0.0f) ? 0.0f : (1.0f - phase_shift_pu);
    }

    uint32_t base_master = pwm_channels[master_ch];
    uint32_t base_slave  = pwm_channels[slave_ch];

    // Guard: periods must match
    uint16_t period_master = EPWM_getTimeBasePeriod(base_master);
    uint16_t period_slave  = EPWM_getTimeBasePeriod(base_slave);

    if (period_master != period_slave) return HAL_ERROR;
    if (period_master == 0)           return HAL_ERROR;

    // Compute TBPHS and count direction
    //
    // In up-down mode the carrier spans 2*TBPRD ticks per cycle:
    //
    //   phase_ticks = shift * 2 * TBPRD
    //
    // TBPHS is limited to [0, TBPRD], so the range splits into two halves:
    //
    //   phase_ticks ∈ [0,      TBPRD]  > TBPHS = phase_ticks,             count UP
    //   phase_ticks ∈ (TBPRD, 2*TBPRD) > TBPHS = 2*TBPRD - phase_ticks,  count DOWN
    //
    // Counting DOWN from a mirrored TBPHS produces the same effective delay
    // as counting UP from the mirror point would in the second half of the
    // cycle.
    //
    uint32_t full_cycle_ticks = 2U * (uint32_t)period_master;
    uint32_t phase_ticks = (uint32_t)(shift * (float)full_cycle_ticks + 0.5f);  // rounded
    if (phase_ticks > full_cycle_ticks) phase_ticks = full_cycle_ticks;          // clamp float rounding

    uint16_t             tbphs;
    EPWM_SyncCountMode   count_mode;

    if (phase_ticks <= (uint32_t)period_master) {
        tbphs      = (uint16_t)phase_ticks;
        count_mode = EPWM_COUNT_MODE_UP_AFTER_SYNC;
    } else {
        tbphs      = (uint16_t)(full_cycle_ticks - phase_ticks);
        count_mode = EPWM_COUNT_MODE_DOWN_AFTER_SYNC;
    }

    // Configure master
    // Master fires SYNCOUT on counter-zero; InitPWM already sets this, but
    // be explicit here in case the master was later reconfigured.
    EPWM_setSyncOutPulseMode(base_master, EPWM_SYNC_OUT_PULSE_ON_COUNTER_ZERO);

    // Pass sync through any intermediate channels
    for (uint32_t ch = master_ch + 1; ch < slave_ch; ch++) {
        EPWM_setSyncOutPulseMode(pwm_channels[ch], EPWM_SYNC_OUT_PULSE_ON_EPWMxSYNCIN);
    }

    // Configure slave
    EPWM_setPhaseShift(base_slave, tbphs);
    EPWM_setCountModeAfterSync(base_slave, count_mode);
    EPWM_enablePhaseShiftLoad(base_slave);   // arm the load-on-SYNCI latch

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
