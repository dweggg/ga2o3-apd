#ifndef __USER_INTERFACE_H__
#define __USER_INTERFACE_H__

#include <stdint.h>

typedef enum {
    UI_MODE_IDLE = 0,
    UI_MODE_RAW_PWM,
    UI_MODE_OPEN_LOOP_VOLTAGE,
    UI_MODE_CLOSED_LOOP_SINGLE,
    UI_MODE_CLOSED_LOOP_INTERLEAVED,
    UI_MODE_BATCH_TEST,
} UiModeTypeDef;

typedef enum {
    PWM_CHANNEL_A = 1U,
    PWM_CHANNEL_B = 2U,
    PWM_CHANNEL_C = 3U,
} PwmChannelTypeDef;

typedef struct {
    uint32_t frequency_hz;
    uint32_t deadtime_ns;
    float    duty_cycle;
} RawPwmSettingsTypeDef;

typedef struct {
    float voltage_amplitude_volts;
    float fundamental_frequency_hz;
} OpenLoopVoltageSettingsTypeDef;

typedef struct {
    float id_reference_amps;
    float iq_reference_amps;
} ClosedLoopSettingsTypeDef;

typedef struct {
    float current_a_amps;
    float current_b_amps;
    float current_c_amps;
    float voltage_a_volts;
    float voltage_b_volts;
    float voltage_c_volts;
    float voltage_dc_volts;
    float temp_ah_celsius;
    float temp_al_celsius;
    float temp_bh_celsius;
    float temp_bl_celsius;
    float temp_ch_celsius;
    float temp_cl_celsius;
} AdcMonitoringTypeDef;

typedef struct {
    UiModeTypeDef current_mode;
    
    RawPwmSettingsTypeDef raw_pwm_a;
    RawPwmSettingsTypeDef raw_pwm_b;
    RawPwmSettingsTypeDef raw_pwm_c;
    PwmChannelTypeDef selected_pwm_channel;
    
    OpenLoopVoltageSettingsTypeDef open_loop;
    
    ClosedLoopSettingsTypeDef closed_loop;
    
    uint16_t use_interleaved_mode;
    
    uint16_t system_enabled;
    
    AdcMonitoringTypeDef adc_monitor;
} UserInterfaceTypeDef;

extern UserInterfaceTypeDef g_ui;

void InitUserInterface(void);

void TaskUserInterface(void);

void UpdateAdcMonitoring(void);

void EnableSystem(void);
void DisableSystem(void);

void SetUIMode(UiModeTypeDef mode);

void UpdateRawPwmChannel(PwmChannelTypeDef channel, uint32_t freq_hz, uint32_t deadtime_ns, float duty);

void UpdateOpenLoopVoltage(float voltage_amplitude, float fundamental_frequency);

void UpdateClosedLoopSetpoints(float id_amps, float iq_amps);

void UpdateInterleaving(uint16_t enable);

void StartBatchTest(void);
uint16_t IsBatchTestRunning(void);

#endif
