#include "user_interface.h"
#include "control_loop.h"
#include "state_machine.h"
#include "adc_config.h"
#include "bsp_epwm.h"
#include "batch.h"
#include "global_defines.h"
#include <string.h>

UserInterfaceTypeDef g_ui = {0};

static uint16_t s_batch_test_running = 0U;

typedef struct {
    RawPwmSettingsTypeDef raw_pwm_a;
    RawPwmSettingsTypeDef raw_pwm_b;
    RawPwmSettingsTypeDef raw_pwm_c;
    OpenLoopVoltageSettingsTypeDef open_loop;
    ClosedLoopSettingsTypeDef closed_loop;
    uint16_t use_interleaved_mode;
} ParameterTrackingTypeDef;

static ParameterTrackingTypeDef s_last_parameters = {0};

/* Force s_last_parameters out of sync so all updates fire on next poll */
static void InvalidateParameterTracking(void)
{
    /* Setting to an impossible/sentinel state ensures every field compares dirty */
    memset(&s_last_parameters, 0xFF, sizeof(s_last_parameters));
}

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

    /* Sync tracking so no spurious updates fire at startup */
    s_last_parameters.raw_pwm_a          = g_ui.raw_pwm_a;
    s_last_parameters.raw_pwm_b          = g_ui.raw_pwm_b;
    s_last_parameters.raw_pwm_c          = g_ui.raw_pwm_c;
    s_last_parameters.open_loop          = g_ui.open_loop;
    s_last_parameters.closed_loop        = g_ui.closed_loop;
    s_last_parameters.use_interleaved_mode = g_ui.use_interleaved_mode;
}

void EnableSystem(void)
{
    if (g_ui.current_mode == UI_MODE_IDLE) {
        return;
    }

    g_ui.system_enabled = 1U;
    ControlLoop_Enable();
}

void DisableSystem(void)
{
    g_ui.system_enabled = 0U;
    ControlLoop_Disable();

    SetDuty(CHANNEL_A, 0.0f);
    SetDuty(CHANNEL_B, 0.0f);
    SetDuty(CHANNEL_C, 0.0f);
}

void SetUIMode(UiModeTypeDef mode)
{
    DisableSystem();
    g_ui.current_mode = mode;

    /*
     * Invalidate tracking so PollAndApplyParameterUpdates will push all
     * current g_ui values to hardware on the next task tick, regardless of
     * what the previous mode was.
     */
    InvalidateParameterTracking();

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
            /* Sync tracking — nothing to push, keep it clean */
            s_last_parameters.raw_pwm_a            = g_ui.raw_pwm_a;
            s_last_parameters.raw_pwm_b            = g_ui.raw_pwm_b;
            s_last_parameters.raw_pwm_c            = g_ui.raw_pwm_c;
            s_last_parameters.open_loop            = g_ui.open_loop;
            s_last_parameters.closed_loop          = g_ui.closed_loop;
            s_last_parameters.use_interleaved_mode = g_ui.use_interleaved_mode;
            break;
    }
}

/* --------------------------------------------------------------------------
 * Raw hardware application helpers — no mode guards, no enable guards.
 * These apply values unconditionally; callers are responsible for context.
 * -------------------------------------------------------------------------- */

static void ApplyRawPwmChannelToHW(PwmChannelTypeDef channel,
                                    const RawPwmSettingsTypeDef *s)
{
    uint32_t hw;
    switch (channel) {
        case PWM_CHANNEL_A: hw = CHANNEL_A; break;
        case PWM_CHANNEL_B: hw = CHANNEL_B; break;
        case PWM_CHANNEL_C: hw = CHANNEL_C; break;
        default: return;
    }
    SetFrequency(hw, s->frequency_hz);
    SetDeadTime(hw, s->deadtime_ns);
    SetDuty(hw, s->duty_cycle);
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
 * Public update functions — update g_ui state only.
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
 * Parameter polling — single authoritative point where g_ui is compared to
 * last-applied state and hardware is updated if the mode permits it.
 * -------------------------------------------------------------------------- */

/* Integer-safe float comparison for dirty detection */
static uint16_t FloatsDiffer(float a, float b)
{
    /* Bitwise compare avoids FP equality pitfalls for stored/recalled values */
    uint32_t ua, ub;
    __builtin_memcpy(&ua, &a, sizeof(ua));
    __builtin_memcpy(&ub, &b, sizeof(ub));
    return (ua != ub) ? 1U : 0U;
}

static void PollAndApplyParameterUpdates(void)
{
    if (!g_ui.system_enabled) {
        /*
         * Don't push parameter changes to hardware while disabled.
         * Sync tracking so we don't accumulate a backlog of "dirty" updates
         * that fire all at once on re-enable.  Re-enable goes through
         * EnableSystem() -> the first post-enable tick will push clean state.
         *
         * Exception: let the tracking stay dirty if the mode itself changed
         * (SetUIMode invalidated tracking) — that dirt will be applied after
         * EnableSystem() is called by the host.
         */
        return;
    }

    switch (g_ui.current_mode) {

        case UI_MODE_RAW_PWM: {
            if (g_ui.raw_pwm_a.frequency_hz != s_last_parameters.raw_pwm_a.frequency_hz ||
                g_ui.raw_pwm_a.deadtime_ns  != s_last_parameters.raw_pwm_a.deadtime_ns  ||
                FloatsDiffer(g_ui.raw_pwm_a.duty_cycle, s_last_parameters.raw_pwm_a.duty_cycle)) {
                ApplyRawPwmChannelToHW(PWM_CHANNEL_A, &g_ui.raw_pwm_a);
                s_last_parameters.raw_pwm_a = g_ui.raw_pwm_a;
            }

            if (g_ui.raw_pwm_b.frequency_hz != s_last_parameters.raw_pwm_b.frequency_hz ||
                g_ui.raw_pwm_b.deadtime_ns  != s_last_parameters.raw_pwm_b.deadtime_ns  ||
                FloatsDiffer(g_ui.raw_pwm_b.duty_cycle, s_last_parameters.raw_pwm_b.duty_cycle)) {
                ApplyRawPwmChannelToHW(PWM_CHANNEL_B, &g_ui.raw_pwm_b);
                s_last_parameters.raw_pwm_b = g_ui.raw_pwm_b;
            }

            if (g_ui.raw_pwm_c.frequency_hz != s_last_parameters.raw_pwm_c.frequency_hz ||
                g_ui.raw_pwm_c.deadtime_ns  != s_last_parameters.raw_pwm_c.deadtime_ns  ||
                FloatsDiffer(g_ui.raw_pwm_c.duty_cycle, s_last_parameters.raw_pwm_c.duty_cycle)) {
                ApplyRawPwmChannelToHW(PWM_CHANNEL_C, &g_ui.raw_pwm_c);
                s_last_parameters.raw_pwm_c = g_ui.raw_pwm_c;
            }
            break;
        }

        case UI_MODE_OPEN_LOOP_VOLTAGE: {
            if (FloatsDiffer(g_ui.open_loop.voltage_amplitude_volts,
                             s_last_parameters.open_loop.voltage_amplitude_volts) ||
                FloatsDiffer(g_ui.open_loop.fundamental_frequency_hz,
                             s_last_parameters.open_loop.fundamental_frequency_hz)) {
                ApplyOpenLoopToHW();
                s_last_parameters.open_loop = g_ui.open_loop;
            }
            break;
        }

        case UI_MODE_CLOSED_LOOP_SINGLE:
        case UI_MODE_CLOSED_LOOP_INTERLEAVED: {
            /* Interleaving change requires a controlled reinit */
            if (g_ui.use_interleaved_mode != s_last_parameters.use_interleaved_mode) {
                DisableSystem();
                ControlLoop_SetInterleavedMode(g_ui.use_interleaved_mode);
                InitControlLoop();
                s_last_parameters.use_interleaved_mode = g_ui.use_interleaved_mode;
                /* Also mark setpoints dirty so they are re-applied after reinit */
                s_last_parameters.closed_loop.id_reference_amps = g_ui.closed_loop.id_reference_amps + 1.0f;
                EnableSystem();
            }

            if (FloatsDiffer(g_ui.closed_loop.id_reference_amps,
                             s_last_parameters.closed_loop.id_reference_amps) ||
                FloatsDiffer(g_ui.closed_loop.iq_reference_amps,
                             s_last_parameters.closed_loop.iq_reference_amps)) {
                ApplyClosedLoopSetpointsToHW();
                s_last_parameters.closed_loop = g_ui.closed_loop;
            }
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
