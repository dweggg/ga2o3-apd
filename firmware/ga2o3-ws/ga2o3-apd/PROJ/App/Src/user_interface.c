/**
 * @file user_interface.c
 * @brief User interface layer: coordinates mode management, parameter updates, and task execution
 * 
 * Responsibilities:
 * - Manage UI modes (raw PWM, open-loop, closed-loop, batch test)
 * - Apply mode-specific parameters to hardware
 * - Route and coordinate batch test operations
 * - Call state machine and safety tasks on each cycle
 * - Monitor ADC values
 */

#include "user_interface.h"
#include "control_loop.h"
#include "state_machine.h"
#include "safety.h"
#include "batch.h"
#include "adc_config.h"
#include "bsp_epwm.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Module state                                                                */
/* -------------------------------------------------------------------------- */

UserInterfaceTypeDef g_ui = {
    .current_mode = (UiModeTypeDef)0,
    .raw_pwm_a = {0},
    .raw_pwm_b = {0},
    .raw_pwm_c = {0},
    .open_loop = {0},
    .closed_loop = {0},
    .use_interleaved_mode = 0,
    .system_enabled = 0,
};

static uint16_t s_batch_test_running = 0U;

/* -------------------------------------------------------------------------- */
/* Initialization                                                              */
/* -------------------------------------------------------------------------- */

void InitUserInterface(void)
{
    g_ui.current_mode = UI_MODE_IDLE;
    g_ui.system_enabled = 0U;

    g_ui.raw_pwm_a.frequency_hz = 10000U;
    g_ui.raw_pwm_a.deadtime_ns  = 1000U;
    g_ui.raw_pwm_a.duty_cycle   = 0.0f;

    g_ui.raw_pwm_b.frequency_hz = 10000U;
    g_ui.raw_pwm_b.deadtime_ns  = 1000U;
    g_ui.raw_pwm_b.duty_cycle   = 0.0f;

    g_ui.raw_pwm_c.frequency_hz = 10000U;
    g_ui.raw_pwm_c.deadtime_ns  = 1000U;
    g_ui.raw_pwm_c.duty_cycle   = 0.0f;

    g_ui.open_loop.voltage_amplitude_volts = 48.0f;
    g_ui.open_loop.fundamental_frequency_hz = 50.0f;

    g_ui.closed_loop.id_reference_amps = 0.0f;
    g_ui.closed_loop.iq_reference_amps = 0.0f;

    g_ui.use_interleaved_mode = 0U;

    InitPWM(PWM_CHANNEL_A);
    InitPWM(PWM_CHANNEL_B);
    InitPWM(PWM_CHANNEL_C);

    GPIO_setPadConfig(GD_ENABLE_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_ENABLE_PIN, GPIO_DIR_MODE_OUT);
    GPIO_writePin(GD_ENABLE_PIN, 0); // off
  
    DisableSystem();
}

void EnableSystem(void)
{
    if (g_ui.current_mode == UI_MODE_IDLE) {
        return;
    }

    g_ui.system_enabled = 1U;
    EnableDrivers();
    ControlLoop_Enable();
    EnablePWM(PWM_CHANNEL_A);
    EnablePWM(PWM_CHANNEL_B);
    EnablePWM(PWM_CHANNEL_C);
}

void DisableSystem(void)
{
    g_ui.system_enabled = 0U;
    DisableDrivers();
    ControlLoop_Disable();

    DisablePWM(PWM_CHANNEL_A);
    DisablePWM(PWM_CHANNEL_B);
    DisablePWM(PWM_CHANNEL_C);
}

//@brief Query system enabled state
//@return 1 if system is enabled, 0 otherwise
uint16_t GetUiSystemEnabled(void)
{
    return g_ui.system_enabled;
}

/* -------------------------------------------------------------------------- */
/* Mode management                                                             */
/* -------------------------------------------------------------------------- */

void SetUIMode(UiModeTypeDef mode)
{
    DisableSystem();
    g_ui.current_mode = mode;

    switch (mode) {
        case UI_MODE_BATCH_TEST:
            s_batch_test_running = 0U;
            InitControlLoop();
            ControlLoop_SetInterleavedMode(0U);
            break;

        case UI_MODE_RAW_PWM:
        case UI_MODE_OPEN_LOOP_AC:
        case UI_MODE_POWER_CYCLING:
        case UI_MODE_POWER_CYCLING_INTERLEAVED:
            // Hardware will be configured by PollAndApplyParameterUpdates
            break;

        case UI_MODE_IDLE:
        default:
            break;
    }
}

/* -------------------------------------------------------------------------- */
/* Raw PWM parameter application                                              */
/* -------------------------------------------------------------------------- */

static void ApplyRawPwmChannelToHW(PwmChannelTypeDef channel,
                                    const RawPwmSettingsTypeDef *s)
{
    // Extract all values before calling hardware functions to prevent register clobbering
    uint32_t freq_hz = s->frequency_hz;
    uint32_t deadtime_ns = s->deadtime_ns;
    float duty_cycle = s->duty_cycle;

    SetFrequency(channel, freq_hz);
    SetDeadTime(channel, deadtime_ns);
    SetDuty(channel, duty_cycle);
}

void UpdateRawPwmChannel(PwmChannelTypeDef channel,
                          uint32_t freq_hz, uint32_t deadtime_ns, float duty)
{
    switch (channel) {
        case PWM_CHANNEL_A:
            g_ui.raw_pwm_a.frequency_hz = freq_hz;
            g_ui.raw_pwm_a.deadtime_ns  = deadtime_ns;
            g_ui.raw_pwm_a.duty_cycle   = duty;
            break;
        case PWM_CHANNEL_B:
            g_ui.raw_pwm_b.frequency_hz = freq_hz;
            g_ui.raw_pwm_b.deadtime_ns  = deadtime_ns;
            g_ui.raw_pwm_b.duty_cycle   = duty;
            break;
        case PWM_CHANNEL_C:
            g_ui.raw_pwm_c.frequency_hz = freq_hz;
            g_ui.raw_pwm_c.deadtime_ns  = deadtime_ns;
            g_ui.raw_pwm_c.duty_cycle   = duty;
            break;
        default:
            break;
    }
    // Hardware will be updated by PollAndApplyParameterUpdates on next tick
}

void UpdateOpenLoopVoltage(float voltage_amplitude, float fundamental_frequency)
{
    g_ui.open_loop.voltage_amplitude_volts  = voltage_amplitude;
    g_ui.open_loop.fundamental_frequency_hz = fundamental_frequency;
}

void UpdateClosedLoopSetpoints(float id_amps, float iq_amps)
{
    g_ui.closed_loop.id_reference_amps = id_amps;
    g_ui.closed_loop.iq_reference_amps = iq_amps;
}


/* -------------------------------------------------------------------------- */
/* Batch test coordination                                                     */
/* -------------------------------------------------------------------------- */

//@brief Apply batch test's current frequency and deadtime settings to hardware
static void ApplyBatchTestSettings(void)
{
    static uint32_t last_mode = 0xFFFFFFFFU;
    uint32_t current_mode = BatchGetCurrentMode();
    
    // Reconfigure interleaving and PWM enable on mode transitions
    if (current_mode != last_mode)
    {
        // Mode changed - reconfigure channel setup
        if (current_mode == 0U)  // ModeSingle
        {
            DisablePWM(PWM_CHANNEL_B);
            EnablePWM(PWM_CHANNEL_A);
            ControlLoop_SetInterleavedMode(0);
        }
        else  // ModeInterleaved
        {
            EnablePWM(PWM_CHANNEL_A);
            EnablePWM(PWM_CHANNEL_B);
            ControlLoop_SetInterleavedMode(1);
        }
        last_mode = current_mode;
    }

    // Apply frequency and deadtime for current index
    uint32_t freq = BatchGetCurrentFrequency();
    uint32_t deadtime = BatchGetCurrentDeadtime();
    
    SetFrequency(PWM_CHANNEL_A, freq);
    SetFrequency(PWM_CHANNEL_B, freq);
    SetDeadTime(PWM_CHANNEL_A, deadtime);
    SetDeadTime(PWM_CHANNEL_B, deadtime);
}

void StartBatchTest(void)
{
    if (g_ui.current_mode != UI_MODE_BATCH_TEST) {
        return;
    }

    s_batch_test_running = 1U;
    StartBatch();
}

uint16_t IsBatchTestRunning(void)
{
    if (g_ui.current_mode != UI_MODE_BATCH_TEST) {
        return 0U;
    }

    return (s_batch_test_running && !IsBatchComplete()) ? 1U : 0U;
}

/* -------------------------------------------------------------------------- */
/* Parameter application - mode-specific hardware configuration               */
/* -------------------------------------------------------------------------- */

static void PollAndApplyParameterUpdates(void)
{
    if (!g_ui.system_enabled) {
        DisableSystem();
        return;
    }

    switch (g_ui.current_mode) {

        case UI_MODE_RAW_PWM: {
            /* Raw PWM mode: direct duty cycle control, control loop disabled */
            ControlLoop_Disable();
            EnableDrivers();
            EnablePWM(PWM_CHANNEL_A);
            EnablePWM(PWM_CHANNEL_B);
            EnablePWM(PWM_CHANNEL_C);
            ApplyRawPwmChannelToHW(PWM_CHANNEL_A, &g_ui.raw_pwm_a);
            ApplyRawPwmChannelToHW(PWM_CHANNEL_B, &g_ui.raw_pwm_b);
            ApplyRawPwmChannelToHW(PWM_CHANNEL_C, &g_ui.raw_pwm_c);
            break;
        }

        case UI_MODE_OPEN_LOOP_AC: {
            /* Open-loop voltage mode: set voltage magnitude and frequency */
            EnableSystem();
            ControlLoop_SetOpenLoopVoltage(
                g_ui.open_loop.voltage_amplitude_volts,
                g_ui.open_loop.fundamental_frequency_hz
            );
            break;
        }

        case UI_MODE_CLOSED_LOOP_BUCK:
            /* Closed-loop current control (DC) */
            ControlLoop_SetInterleavedMode(0);
            ControlLoop_SetBuckMode(1);
            EnableSystem();
            ControlLoop_SetIdRef(g_ui.closed_loop.id_reference_amps);
            ControlLoop_SetIqRef(g_ui.closed_loop.iq_reference_amps);
            break;
        case UI_MODE_POWER_CYCLING:
            /* Closed-loop current control and power cycling */
            ControlLoop_SetInterleavedMode(0);
            ControlLoop_SetBuckMode(0);
            EnableSystem();
            ControlLoop_SetIdRef(g_ui.closed_loop.id_reference_amps);
            ControlLoop_SetIqRef(g_ui.closed_loop.iq_reference_amps);
            break;
        case UI_MODE_POWER_CYCLING_INTERLEAVED: {
            /* Closed-loop current control and power cycling with interleaving */
            ControlLoop_SetInterleavedMode(1);
            EnableSystem();
            ControlLoop_SetIdRef(g_ui.closed_loop.id_reference_amps);
            ControlLoop_SetIqRef(g_ui.closed_loop.iq_reference_amps);
            break;
        }

        case UI_MODE_BATCH_TEST: {
            /* Batch test: apply frequency/deadtime from batch state */
            if (s_batch_test_running && !IsBatchComplete()) {
                ApplyBatchTestSettings();
            }
            break;
        }

        case UI_MODE_IDLE:
        default:
            break;
    }
}

/* -------------------------------------------------------------------------- */
/* Main task - called every control cycle                                     */
/* -------------------------------------------------------------------------- */

void TaskUserInterface(void)
{

    // Apply current mode's parameter settings
    PollAndApplyParameterUpdates();

    // Execute batch test state machine if running
    if (g_ui.current_mode == UI_MODE_BATCH_TEST) {
        if (s_batch_test_running) {
            RunTests();
            if (IsBatchComplete()) {
                s_batch_test_running = 0U;
                DisableSystem();
            }
        }
    }
}
