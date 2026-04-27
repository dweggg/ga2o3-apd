/*
 * adc_config.h
 * Channel mapping and physical measurement API for F28379D
 * Author: David Redondo - 2026
 */

#ifndef ADC_CONFIG_H
#define ADC_CONFIG_H

#include <stdint.h>
#include "bsp_adc.h"

/* -----------------------------------------------------------------------
 * Channel map  -  one row per physical measurement
 *
 *   MODULE   : ADC module  (1=A, 2=B, 3=C, 4=D)
 *   SOC      : SOC slot    (0–15, unique per module)
 *   CHANNEL  : DriverLib ADC_Channel_* enum for that pin
 *
 * SOC numbers must be unique within each module; they can repeat across
 * different modules.
 * ----------------------------------------------------------------------- */


// ---------------------- TEMPERATURES ----------------------
// AH -> ADC Module C, IN3
#define TEMP_AH_ADC_MODULE        3                 // ADC C
#define TEMP_AH_ADC_SOC           0                 // ADCC-SOC0
#define TEMP_AH_ADC_CHANNEL       ADC_CH_ADCIN3     // ADCC pin IN3

// AL -> ADC Module C, IN14
#define TEMP_AL_ADC_MODULE        3                 // ADC C
#define TEMP_AL_ADC_SOC           1                 // ADCC-SOC0
#define TEMP_AL_ADC_CHANNEL       ADC_CH_ADCIN14    // ADCC pin IN14

// BH -> ADC Module C, IN5
#define TEMP_BH_ADC_MODULE        3                 // ADC C
#define TEMP_BH_ADC_SOC           2                 // ADCC-SOC1
#define TEMP_BH_ADC_CHANNEL       ADC_CH_ADCIN5     // ADCC pin IN5

// BL -> ADC Module C, IN15
#define TEMP_BL_ADC_MODULE        3                 // ADC C
#define TEMP_BL_ADC_SOC           3                 // ADCC-SOC1
#define TEMP_BL_ADC_CHANNEL       ADC_CH_ADCIN15    // ADCC pin IN15

// CH -> ADC Module C, IN4
#define TEMP_CH_ADC_MODULE        3                 // ADC C
#define TEMP_CH_ADC_SOC           4                 // ADCC-SOC2
#define TEMP_CH_ADC_CHANNEL       ADC_CH_ADCIN4     // ADCC pin IN4

// CL -> ADC Module C, IN2
#define TEMP_CL_ADC_MODULE        3                 // ADC C
#define TEMP_CL_ADC_SOC           5                 // ADCC-SOC3
#define TEMP_CL_ADC_CHANNEL       ADC_CH_ADCIN2     // ADCC pin IN2


// ---------------------- VOLTAGES ----------------------
// Va -> ADC Module B, IN3
#define V_A_ADC_MODULE            2                 // ADC B
#define V_A_ADC_SOC               0                 // ADCB-SOC0
#define V_A_ADC_CHANNEL           ADC_CH_ADCIN3     // ADCB pin IN3

// Vb -> ADC Module B, IN5
#define V_B_ADC_MODULE            2                 // ADC B
#define V_B_ADC_SOC               1                 // ADCB-SOC1
#define V_B_ADC_CHANNEL           ADC_CH_ADCIN5     // ADCB pin IN5

// Vc -> ADC Module B, IN4
#define V_C_ADC_MODULE            2                 // ADC B
#define V_C_ADC_SOC               2                 // ADCB-SOC2
#define V_C_ADC_CHANNEL           ADC_CH_ADCIN4     // ADCB pin IN4

// V_dc -> ADC Module B, IN2
#define V_DC_ADC_MODULE           2                 // ADC B
#define V_DC_ADC_SOC              3                 // ADCB-SOC3
#define V_DC_ADC_CHANNEL          ADC_CH_ADCIN2     // ADCB pin IN2


// ---------------------- CURRENTS ----------------------
// Ia -> ADC Module A, IN0
#define I_A_ADC_MODULE            1                 // ADC A
#define I_A_ADC_SOC               0                 // ADCA-SOC2
#define I_A_ADC_CHANNEL           ADC_CH_ADCIN3     // ADCA pin IN0

// Ib -> ADC Module A, IN2
#define I_B_ADC_MODULE            1                 // ADC A
#define I_B_ADC_SOC               1                 // ADCA-SOC3
#define I_B_ADC_CHANNEL           ADC_CH_ADCIN2     // ADCA pin IN2

// Ic -> ADC Module A, IN5
#define I_C_ADC_MODULE            1                 // ADC A
#define I_C_ADC_SOC               2                 // ADCA-SOC4
#define I_C_ADC_CHANNEL           ADC_CH_ADCIN5     // ADCA pin IN5


/* -----------------------------------------------------------------------
 * Shared ADC settings
 * ----------------------------------------------------------------------- */

#define TEMP_SAMPLE_WINDOW_NS       20U // ns

#define VOLTAGE_GAIN                -0.33f
#define VOLTAGE_OFFSET              650.1f
#define VOLTAGE_SAMPLE_WINDOW_NS    20U // ns

#define CURRENT_GAIN                0.03174603f
#define CURRENT_OFFSET              -62.5f
#define CURRENT_SAMPLE_WINDOW_NS    20U // ns


/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/* Initialise every ADC module and configure every SOC listed above. */
HAL_StatusTypeDef InitConfigADC(void);
HAL_StatusTypeDef CalibrateCurrentOffset(uint16_t num_samples);

void TriggerTempADC(void);
void TriggerVoltageADC(void);

float GetTempAH(void);
float GetTempAL(void);
float GetTempBH(void);
float GetTempBL(void);
float GetTempCH(void);
float GetTempCL(void);

float GetVoltageA(void);
float GetVoltageB(void);
float GetVoltageC(void);
float GetVoltageDC(void);

float GetCurrentA(void);
float GetCurrentB(void);
float GetCurrentC(void);

#endif /* ADC_CONFIG_H */
