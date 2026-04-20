#include "user_interface.h"
#include "control_loop.h"
#include "state_machine.h"
#include "adc_config.h"
#include "bsp_epwm.h"
#include "batch.h"
#include "global_defines.h"

UserInterfaceTypeDef g_ui = {0};

static uint16_t s_batch_test_running = 0U;

/* Tracking structure for parameter changes detected during task execution */
typedef struct {
    RawPwmSettingsTypeDef raw_pwm_a;
    RawPwmSettingsTypeDef raw_pwm_b;
    RawPwmSettingsTypeDef raw_pwm_c;
    OpenLoopVoltageSettingsTypeDef open_loop;
    ClosedLoopSettingsTypeDef closed_loop;
    uint16_t use_interleaved_mode;
} ParameterTrackingTypeDef;

static ParameterTrackingTypeDef s_last_parameters = {0};

void InitUserInterface(void)
{
    g_ui.current_mode = UI_MODE_IDLE;
    g_ui.system_enabled = 0U;
    
    g_ui.raw_pwm_a.frequency_hz = 10000U;
    g_ui.raw_pwm_a.deadtime_ns = 1000U;
    g_ui.raw_pwm_a.duty_cycle = 0.0f;
    
    g_ui.raw_pwm_b.frequency_hz = 10000U;
    g_ui.raw_pwm_b.deadtime_ns = 1000U;
    g_ui.raw_pwm_b.duty_cycle = 0.0f;
    
    g_ui.raw_pwm_c.frequency_hz = 10000U;
    g_ui.raw_pwm_c.deadtime_ns = 1000U;
    g_ui.raw_pwm_c.duty_cycle = 0.0f;
    
    g_ui.selected_pwm_channel = PWM_CHANNEL_A;
    
    g_ui.open_loop.voltage_amplitude_volts = 48.0f;
    g_ui.open_loop.fundamental_frequency_hz = 50.0f;
    
    g_ui.closed_loop.id_reference_amps = 0.0f;
    g_ui.closed_loop.iq_reference_amps = 0.0f;
    
    g_ui.use_interleaved_mode = 0U;
    
    /* Initialize parameter tracking */
    s_last_parameters.raw_pwm_a = g_ui.raw_pwm_a;
    s_last_parameters.raw_pwm_b = g_ui.raw_pwm_b;
    s_last_parameters.raw_pwm_c = g_ui.raw_pwm_c;
    s_last_parameters.open_loop = g_ui.open_loop;
    s_last_parameters.closed_loop = g_ui.closed_loop;
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
    
    switch (mode) {
        case UI_MODE_RAW_PWM:
            SetFrequency(CHANNEL_A, g_ui.raw_pwm_a.frequency_hz);
            SetFrequency(CHANNEL_B, g_ui.raw_pwm_b.frequency_hz);
            SetFrequency(CHANNEL_C, g_ui.raw_pwm_c.frequency_hz);
            
            SetDeadTime(CHANNEL_A, g_ui.raw_pwm_a.deadtime_ns);
            SetDeadTime(CHANNEL_B, g_ui.raw_pwm_b.deadtime_ns);
            SetDeadTime(CHANNEL_C, g_ui.raw_pwm_c.deadtime_ns);
            
            SetDuty(CHANNEL_A, g_ui.raw_pwm_a.duty_cycle);
            SetDuty(CHANNEL_B, g_ui.raw_pwm_b.duty_cycle);
            SetDuty(CHANNEL_C, g_ui.raw_pwm_c.duty_cycle);
            break;
            
        case UI_MODE_OPEN_LOOP_VOLTAGE:
            ControlLoop_SetOpenLoopVoltage(
                g_ui.open_loop.voltage_amplitude_volts,
                g_ui.open_loop.fundamental_frequency_hz
            );
            break;
            
        case UI_MODE_CLOSED_LOOP_SINGLE:
            ControlLoop_SetInterleavedMode(0U);
            ControlLoop_SetIdRef(g_ui.closed_loop.id_reference_amps);
            ControlLoop_SetIqRef(g_ui.closed_loop.iq_reference_amps);
            break;
            
        case UI_MODE_CLOSED_LOOP_INTERLEAVED:
            ControlLoop_SetInterleavedMode(1U);
            ControlLoop_SetIdRef(g_ui.closed_loop.id_reference_amps);
            ControlLoop_SetIqRef(g_ui.closed_loop.iq_reference_amps);
            break;
            
        case UI_MODE_BATCH_TEST:
            s_batch_test_running = 0U;
            InitControlLoop();
            ControlLoop_SetInterleavedMode(0U);
            break;
            
        case UI_MODE_IDLE:
        default:
            break;
    }
}

void UpdateRawPwmChannel(PwmChannelTypeDef channel, uint32_t freq_hz, uint32_t deadtime_ns, float duty)
{
    if (g_ui.current_mode != UI_MODE_RAW_PWM) {
        return;
    }
    
    uint32_t hw_channel = (uint32_t)channel;
    
    switch (channel) {
        case PWM_CHANNEL_A:
            g_ui.raw_pwm_a.frequency_hz = freq_hz;
            g_ui.raw_pwm_a.deadtime_ns = deadtime_ns;
            g_ui.raw_pwm_a.duty_cycle = duty;
            break;
        case PWM_CHANNEL_B:
            g_ui.raw_pwm_b.frequency_hz = freq_hz;
            g_ui.raw_pwm_b.deadtime_ns = deadtime_ns;
            g_ui.raw_pwm_b.duty_cycle = duty;
            break;
        case PWM_CHANNEL_C:
            g_ui.raw_pwm_c.frequency_hz = freq_hz;
            g_ui.raw_pwm_c.deadtime_ns = deadtime_ns;
            g_ui.raw_pwm_c.duty_cycle = duty;
            break;
    }
    
    if (g_ui.system_enabled) {
        SetFrequency(hw_channel, freq_hz);
        SetDeadTime(hw_channel, deadtime_ns);
        SetDuty(hw_channel, duty);
    }
}

void UpdateOpenLoopVoltage(float voltage_amplitude, float fundamental_frequency)
{
    if (g_ui.current_mode != UI_MODE_OPEN_LOOP_VOLTAGE) {
        return;
    }
    
    g_ui.open_loop.voltage_amplitude_volts = voltage_amplitude;
    g_ui.open_loop.fundamental_frequency_hz = fundamental_frequency;
    
    if (g_ui.system_enabled) {
        ControlLoop_SetOpenLoopVoltage(voltage_amplitude, fundamental_frequency);
    }
}

void UpdateClosedLoopSetpoints(float id_amps, float iq_amps)
{
    if (g_ui.current_mode != UI_MODE_CLOSED_LOOP_SINGLE && 
        g_ui.current_mode != UI_MODE_CLOSED_LOOP_INTERLEAVED) {
        return;
    }
    
    g_ui.closed_loop.id_reference_amps = id_amps;
    g_ui.closed_loop.iq_reference_amps = iq_amps;
    
    if (g_ui.system_enabled) {
        ControlLoop_SetIdRef(id_amps);
        ControlLoop_SetIqRef(iq_amps);
    }
}

void UpdateInterleaving(uint16_t enable)
{
    if (g_ui.current_mode != UI_MODE_CLOSED_LOOP_INTERLEAVED && 
        g_ui.current_mode != UI_MODE_CLOSED_LOOP_SINGLE) {
        return;
    }
    
    g_ui.use_interleaved_mode = enable ? 1U : 0U;
    
    if (g_ui.system_enabled) {
        DisableSystem();
        ControlLoop_SetInterleavedMode(g_ui.use_interleaved_mode);
        InitControlLoop();
        EnableSystem();
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
    
    return s_batch_test_running && !IsBatchComplete();
}

static void PollAndApplyParameterUpdates(void)
{
    /* Check and apply raw PWM channel A updates */
    if (g_ui.raw_pwm_a.frequency_hz != s_last_parameters.raw_pwm_a.frequency_hz ||
        g_ui.raw_pwm_a.deadtime_ns != s_last_parameters.raw_pwm_a.deadtime_ns ||
        g_ui.raw_pwm_a.duty_cycle != s_last_parameters.raw_pwm_a.duty_cycle) {
        UpdateRawPwmChannel(PWM_CHANNEL_A, 
                          g_ui.raw_pwm_a.frequency_hz,
                          g_ui.raw_pwm_a.deadtime_ns,
                          g_ui.raw_pwm_a.duty_cycle);
        s_last_parameters.raw_pwm_a = g_ui.raw_pwm_a;
    }
    
    /* Check and apply raw PWM channel B updates */
    if (g_ui.raw_pwm_b.frequency_hz != s_last_parameters.raw_pwm_b.frequency_hz ||
        g_ui.raw_pwm_b.deadtime_ns != s_last_parameters.raw_pwm_b.deadtime_ns ||
        g_ui.raw_pwm_b.duty_cycle != s_last_parameters.raw_pwm_b.duty_cycle) {
        UpdateRawPwmChannel(PWM_CHANNEL_B,
                          g_ui.raw_pwm_b.frequency_hz,
                          g_ui.raw_pwm_b.deadtime_ns,
                          g_ui.raw_pwm_b.duty_cycle);
        s_last_parameters.raw_pwm_b = g_ui.raw_pwm_b;
    }
    
    /* Check and apply raw PWM channel C updates */
    if (g_ui.raw_pwm_c.frequency_hz != s_last_parameters.raw_pwm_c.frequency_hz ||
        g_ui.raw_pwm_c.deadtime_ns != s_last_parameters.raw_pwm_c.deadtime_ns ||
        g_ui.raw_pwm_c.duty_cycle != s_last_parameters.raw_pwm_c.duty_cycle) {
        UpdateRawPwmChannel(PWM_CHANNEL_C,
                          g_ui.raw_pwm_c.frequency_hz,
                          g_ui.raw_pwm_c.deadtime_ns,
                          g_ui.raw_pwm_c.duty_cycle);
        s_last_parameters.raw_pwm_c = g_ui.raw_pwm_c;
    }
    
    /* Check and apply open loop voltage updates */
    if (g_ui.open_loop.voltage_amplitude_volts != s_last_parameters.open_loop.voltage_amplitude_volts ||
        g_ui.open_loop.fundamental_frequency_hz != s_last_parameters.open_loop.fundamental_frequency_hz) {
        UpdateOpenLoopVoltage(g_ui.open_loop.voltage_amplitude_volts,
                            g_ui.open_loop.fundamental_frequency_hz);
        s_last_parameters.open_loop = g_ui.open_loop;
    }
    
    /* Check and apply closed loop setpoint updates */
    if (g_ui.closed_loop.id_reference_amps != s_last_parameters.closed_loop.id_reference_amps ||
        g_ui.closed_loop.iq_reference_amps != s_last_parameters.closed_loop.iq_reference_amps) {
        UpdateClosedLoopSetpoints(g_ui.closed_loop.id_reference_amps,
                                 g_ui.closed_loop.iq_reference_amps);
        s_last_parameters.closed_loop = g_ui.closed_loop;
    }
    
    /* Check and apply interleaving mode updates */
    if (g_ui.use_interleaved_mode != s_last_parameters.use_interleaved_mode) {
        UpdateInterleaving(g_ui.use_interleaved_mode);
        s_last_parameters.use_interleaved_mode = g_ui.use_interleaved_mode;
    }
}

void TaskUserInterface(void)
{
    UpdateAdcMonitoring();
    PollAndApplyParameterUpdates();
    
    switch (g_ui.current_mode) {
        case UI_MODE_RAW_PWM:
            break;
            
        case UI_MODE_OPEN_LOOP_VOLTAGE:
            break;
            
        case UI_MODE_CLOSED_LOOP_SINGLE:
        case UI_MODE_CLOSED_LOOP_INTERLEAVED:
            break;
            
        case UI_MODE_BATCH_TEST:
            if (s_batch_test_running) {
                RunTests();
                if (IsBatchComplete()) {
                    s_batch_test_running = 0U;
                    DisableSystem();
                }
            }
            break;
            
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
    
    g_ui.adc_monitor.voltage_a_volts = GetVoltageA();
    g_ui.adc_monitor.voltage_b_volts = GetVoltageB();
    g_ui.adc_monitor.voltage_c_volts = GetVoltageC();
    g_ui.adc_monitor.voltage_dc_volts = GetVoltageDC();
    
    g_ui.adc_monitor.temp_ah_celsius = GetTempAH();
    g_ui.adc_monitor.temp_al_celsius = GetTempAL();
    g_ui.adc_monitor.temp_bh_celsius = GetTempBH();
    g_ui.adc_monitor.temp_bl_celsius = GetTempBL();
    g_ui.adc_monitor.temp_ch_celsius = GetTempCH();
    g_ui.adc_monitor.temp_cl_celsius = GetTempCL();
}
