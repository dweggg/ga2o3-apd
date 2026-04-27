#include "user_interface.h"
#include "control_loop.h"
#include "state_machine.h"
#include "adc_config.h"
#include "bsp_epwm.h"
#include "batch.h"
#include <string.h>

UserInterfaceTypeDef g_ui = {
    .current_mode = (UiModeTypeDef)0,
    .raw_pwm_a = {0},
    .raw_pwm_b = {0},
    .raw_pwm_c = {0},
    .selected_pwm_channel = (PwmChannelTypeDef)0,
    .open_loop = {0},
    .closed_loop = {0},
    .use_interleaved_mode = 0,
    .system_enabled = 0,
    .adc_monitor = {0}
};

static uint16_t s_batch_test_running = 0U;



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

    g_ui.selected_pwm_channel = PWM_CHANNEL_A;

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

void EnableDrivers(){
    GPIO_writePin(GD_ENABLE_PIN,1);
}

void DisableDrivers(){
    GPIO_writePin(GD_ENABLE_PIN,0);
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
        case UI_MODE_OPEN_LOOP_VOLTAGE:
        case UI_MODE_CLOSED_LOOP_SINGLE:
        case UI_MODE_CLOSED_LOOP_INTERLEAVED:
            /*
             * Hardware will be configured by PollAndApplyParameterUpdates on
             * the next tick. No need to duplicate the logic here.
             */
            break;

        case UI_MODE_IDLE:
        default:
            break;
    }
}

/* --------------------------------------------------------------------------
 * Raw hardware application helpers - no mode guards, no enable guards.
 * These apply values unconditionally; callers are responsible for context.
 * -------------------------------------------------------------------------- */

static void ApplyRawPwmChannelToHW(PwmChannelTypeDef channel,
                                    const RawPwmSettingsTypeDef *s)
{
    SetFrequency(channel, s->frequency_hz);
    SetDeadTime(channel, s->deadtime_ns);
    SetDuty(channel, s->duty_cycle);
}

static void ApplyOpenLoopToHW(void)
{
    ControlLoop_SetOpenLoopVoltage(
        g_ui.open_loop.voltage_amplitude_volts,
        g_ui.open_loop.fundamental_frequency_hz
    );
}

static void ApplyClosedLoopSetpointsToHW(void)
{
    ControlLoop_SetIdRef(g_ui.closed_loop.id_reference_amps);
    ControlLoop_SetIqRef(g_ui.closed_loop.iq_reference_amps);
}

/* --------------------------------------------------------------------------
 * Public update functions - update g_ui state only.
 * Hardware application is handled exclusively by PollAndApplyParameterUpdates.
 * -------------------------------------------------------------------------- */

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
    /* Hardware will be updated by PollAndApplyParameterUpdates on next tick */
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

void UpdateInterleaving(uint16_t enable)
{
    g_ui.use_interleaved_mode = enable ? 1U : 0U;
}

/* --------------------------------------------------------------------------
 * Parameter application - unconditional raw application on each poll cycle.
 * -------------------------------------------------------------------------- */

static void PollAndApplyParameterUpdates(void)
{
    if (!g_ui.system_enabled) {
        DisableSystem();
        return;
    }

    switch (g_ui.current_mode) {

        case UI_MODE_RAW_PWM: {
            /*
             * Raw PWM drives the switches directly - the control loop must
             * stay disabled so it cannot interfere with the manual duty cycles.
             */
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

        case UI_MODE_OPEN_LOOP_VOLTAGE: {
            EnableSystem();
            ApplyOpenLoopToHW();
            break;
        }

        case UI_MODE_CLOSED_LOOP_SINGLE:
        case UI_MODE_CLOSED_LOOP_INTERLEAVED: {
            EnableSystem();
            /* Interleaving change requires a controlled reinit */
            static uint16_t s_last_interleaved_mode = 0xFFFFU;
            if (g_ui.use_interleaved_mode != s_last_interleaved_mode) {
                DisableSystem();
                ControlLoop_SetInterleavedMode(g_ui.use_interleaved_mode);
                InitControlLoop();
                s_last_interleaved_mode = g_ui.use_interleaved_mode;
                EnableSystem();
            }

            ApplyClosedLoopSetpointsToHW();
            break;
        }

        case UI_MODE_BATCH_TEST:
        case UI_MODE_IDLE:
        default:
            break;
    }
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

void TaskUserInterface(void)
{
    UpdateAdcMonitoring();
    PollAndApplyParameterUpdates();

    switch (g_ui.current_mode) {
        case UI_MODE_BATCH_TEST:
            if (s_batch_test_running) {
                RunTests();
                if (IsBatchComplete()) {
                    s_batch_test_running = 0U;
                    DisableSystem();
                }
            }
            break;

        case UI_MODE_RAW_PWM:
        case UI_MODE_OPEN_LOOP_VOLTAGE:
        case UI_MODE_CLOSED_LOOP_SINGLE:
        case UI_MODE_CLOSED_LOOP_INTERLEAVED:
        case UI_MODE_IDLE:
        default:
            break;
    }
}

void UpdateAdcMonitoring(void)
{
    g_ui.adc_monitor.current_a_amps = GetCurrentA();
    g_ui.adc_monitor.current_b_amps = GetCurrentB();
    g_ui.adc_monitor.current_c_amps = GetCurrentC();

    g_ui.adc_monitor.voltage_a_volts  = GetVoltageA();
    g_ui.adc_monitor.voltage_b_volts  = GetVoltageB();
    g_ui.adc_monitor.voltage_c_volts  = GetVoltageC();
    g_ui.adc_monitor.voltage_dc_volts = GetVoltageDC();

    g_ui.adc_monitor.temp_ah_celsius = GetTempAH();
    g_ui.adc_monitor.temp_al_celsius = GetTempAL();
    g_ui.adc_monitor.temp_bh_celsius = GetTempBH();
    g_ui.adc_monitor.temp_bl_celsius = GetTempBL();
    g_ui.adc_monitor.temp_ch_celsius = GetTempCH();
    g_ui.adc_monitor.temp_cl_celsius = GetTempCL();
}