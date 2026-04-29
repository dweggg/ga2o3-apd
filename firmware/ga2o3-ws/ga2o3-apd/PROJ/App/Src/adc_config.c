/*
 * adc_config.c
 * Channel mapping and physical measurement API for F28379D
 * Author: David Redondo - 2026
 */

#include "adc_config.h"

static AdcTypeDef adc = {0};

typedef struct { uint16_t raw; float temp_c; } TempLutEntry;

static const float VOLTAGE_GAIN = -1.0f/3.32f;
static const float VOLTAGE_OFFSET = -1968.0f;
static const float CURRENT_GAIN = -(1.0f/32.6f) * 0.84f;
/* Mutable current offset for self-calibration */
static float current_offsets[3] = { -2055.0f, -1935.0f, -2012.0f };

static const TempLutEntry temp_lut[] = {
    /*  raw    degC   */
    {  735,    20.5f },
    { 1515,    28.0f },
    { 1980,    33.0f },
    { 2510,    42.0f },
    { 2960,    52.0f },
    { 3200,    61.0f },
    { 3400,    70.0f },
    { 3540,    80.0f },
    { 3620,    86.0f },
    { 3650,    90.0f }
};

#define TEMP_LUT_LEN  (sizeof(temp_lut) / sizeof(temp_lut[0]))


/* -----------------------------------------------------------------------
 * Private helpers
 * ----------------------------------------------------------------------- */

/*
 * Self-calibration routine for current offset.
 *
 * Must be called AFTER InitConfigADC() has configured all current SOCs.
 *
 * @param [in] num_samples       Number of samples to average (recommended: 8-16)
 * @return                       HAL_OK on success, HAL_ERROR if sampling fails
 */
HAL_StatusTypeDef CalibrateCurrentOffset(uint16_t num_samples)
{
    if (num_samples == 0) return HAL_ERROR;

    uint32_t sum_a = 0, sum_b = 0, sum_c = 0;

    for (uint16_t i = 0; i < num_samples; i++) {
        /* Trigger current measurements via software */
        if (SoftwareTriggerSOC(I_A_ADC_MODULE, I_A_ADC_SOC) != HAL_OK)
            return HAL_ERROR;
        if (SoftwareTriggerSOC(I_B_ADC_MODULE, I_B_ADC_SOC) != HAL_OK)
            return HAL_ERROR;
        if (SoftwareTriggerSOC(I_C_ADC_MODULE, I_C_ADC_SOC) != HAL_OK)
            return HAL_ERROR;

        /* Accumulate raw readings */
        sum_a += GetADCResult(I_A_ADC_MODULE, I_A_ADC_SOC);
        sum_b += GetADCResult(I_B_ADC_MODULE, I_B_ADC_SOC);
        sum_c += GetADCResult(I_C_ADC_MODULE, I_C_ADC_SOC);
    }

    /* Compute average raw values and convert to physical offset */
    float avg_raw_a = (float)sum_a / (float)num_samples;
    float avg_raw_b = (float)sum_b / (float)num_samples;
    float avg_raw_c = (float)sum_c / (float)num_samples;

    current_offsets[0] = -avg_raw_a;
    current_offsets[1] = -avg_raw_b;
    current_offsets[2] = -avg_raw_c;

    return HAL_OK;
}

/*
 * Linear interpolation between two LUT points.
 * Returns the y value (temperature) corresponding to x (raw count).
 */
static float lut_interpolate(const TempLutEntry *lo, const TempLutEntry *hi,
                              uint16_t raw)
{
    float t = (float)(raw - lo->raw) / (float)(hi->raw - lo->raw);
    return lo->temp_c + t * (hi->temp_c - lo->temp_c);
}

/*
 * Look up a raw ADC count in the NTC LUT and return degrees Celsius.
 * Clamps to the table endpoints if the raw value falls outside the range.
 */
static float raw_to_celsius(uint16_t raw, const TempLutEntry *lut, uint16_t len)
{
    /* Clamp below table */
    if (raw <= lut[0].raw)       return lut[0].temp_c;
    /* Clamp above table */
    if (raw >= lut[len - 1].raw) return lut[len - 1].temp_c;

    /* Binary search for the bracketing pair */
    uint16_t lo = 0, hi = len - 1;
    while (hi - lo > 1U) {
        uint16_t mid = (lo + hi) / 2U;
        if (raw >= lut[mid].raw) lo = mid;
        else                     hi = mid;
    }

    return lut_interpolate(&lut[lo], &lut[hi], raw);
}




static MovingAvgU16TypeDef temp_ma[6];
static MovingAvgU16TypeDef volt_ma[4];

static uint16_t MovingAvgU16_Push(uint16_t window, MovingAvgU16TypeDef *ma, uint16_t sample)
{
    if (window == 0U || window > (uint16_t)(sizeof(ma->buf) / sizeof(ma->buf[0]))) {
        return sample;   // or 0U, or assert(false)
    }

    if (ma->count < window) {
        ma->buf[ma->idx] = sample;
        ma->sum += sample;
        ma->count++;
    } else {
        ma->sum -= ma->buf[ma->idx];
        ma->buf[ma->idx] = sample;
        ma->sum += sample;
    }

    ma->idx++;
    if (ma->idx >= window) {
        ma->idx = 0U;
    }

    return (uint16_t)(ma->sum / ma->count);
}

/* -----------------------------------------------------------------------
 * Core API
 * ----------------------------------------------------------------------- */

HAL_StatusTypeDef InitConfigADC(void)
{
    HAL_StatusTypeDef status;

    /* --- Initialise modules ------------------------------------------- */

    status = InitADC(TEMP_AH_ADC_MODULE);           // Module C (temps)
    if (status != HAL_OK) return status;

    status = InitADC(V_A_ADC_MODULE);               // Module B (voltages)
    if (status != HAL_OK) return status;

    status = InitADC(I_A_ADC_MODULE);               // Module A (currents)
    if (status != HAL_OK) return status;


    /* --- Configure SOCs: Temperatures (SW trigger) -------------------- */

    status = ConfigureSOC(TEMP_AH_ADC_MODULE,
                          TEMP_AH_ADC_SOC,
                          TEMP_AH_ADC_CHANNEL,
                          ADC_TRIGGER_SW_ONLY,
                          TEMP_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    status = ConfigureSOC(TEMP_AL_ADC_MODULE,
                          TEMP_AL_ADC_SOC,
                          TEMP_AL_ADC_CHANNEL,
                          ADC_TRIGGER_SW_ONLY,
                          TEMP_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    status = ConfigureSOC(TEMP_BH_ADC_MODULE,
                          TEMP_BH_ADC_SOC,
                          TEMP_BH_ADC_CHANNEL,
                          ADC_TRIGGER_SW_ONLY,
                          TEMP_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    status = ConfigureSOC(TEMP_BL_ADC_MODULE,
                          TEMP_BL_ADC_SOC,
                          TEMP_BL_ADC_CHANNEL,
                          ADC_TRIGGER_SW_ONLY,
                          TEMP_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    status = ConfigureSOC(TEMP_CH_ADC_MODULE,
                          TEMP_CH_ADC_SOC,
                          TEMP_CH_ADC_CHANNEL,
                          ADC_TRIGGER_SW_ONLY,
                          TEMP_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    status = ConfigureSOC(TEMP_CL_ADC_MODULE,
                          TEMP_CL_ADC_SOC,
                          TEMP_CL_ADC_CHANNEL,
                          ADC_TRIGGER_SW_ONLY,
                          TEMP_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;


    /* --- Configure SOCs: Voltages (SW trigger) ------------------------ */

    status = ConfigureSOC(V_A_ADC_MODULE,
                          V_A_ADC_SOC,
                          V_A_ADC_CHANNEL,
                          ADC_TRIGGER_SW_ONLY,
                          VOLTAGE_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    status = ConfigureSOC(V_B_ADC_MODULE,
                          V_B_ADC_SOC,
                          V_B_ADC_CHANNEL,
                          ADC_TRIGGER_SW_ONLY,
                          VOLTAGE_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    status = ConfigureSOC(V_C_ADC_MODULE,
                          V_C_ADC_SOC,
                          V_C_ADC_CHANNEL,
                          ADC_TRIGGER_SW_ONLY,
                          VOLTAGE_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    status = ConfigureSOC(V_DC_ADC_MODULE,
                          V_DC_ADC_SOC,
                          V_DC_ADC_CHANNEL,
                          ADC_TRIGGER_SW_ONLY,
                          VOLTAGE_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;


    /* --- Configure SOCs: Currents (EPWM SOCA trigger per phase) ------- */

    status = ConfigureSOC(I_A_ADC_MODULE,
                          I_A_ADC_SOC,
                          I_A_ADC_CHANNEL,
                          ADC_TRIGGER_EPWM1_SOCA,
                          CURRENT_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    status = ConfigureSOC(I_B_ADC_MODULE,
                          I_B_ADC_SOC,
                          I_B_ADC_CHANNEL,
                          ADC_TRIGGER_EPWM2_SOCA,
                          CURRENT_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    status = ConfigureSOC(I_C_ADC_MODULE,
                          I_C_ADC_SOC,
                          I_C_ADC_CHANNEL,
                          ADC_TRIGGER_EPWM3_SOCA,
                          CURRENT_SAMPLE_WINDOW_NS);
    if (status != HAL_OK) return status;

    // CalibrateCurrentOffset(1000);
    return HAL_OK;
}

void TriggerTempADC(void)
{
    SoftwareTriggerSOC(TEMP_AH_ADC_MODULE,    TEMP_AH_ADC_SOC);
    SoftwareTriggerSOC(TEMP_AL_ADC_MODULE,    TEMP_AL_ADC_SOC);
    SoftwareTriggerSOC(TEMP_BH_ADC_MODULE,    TEMP_BH_ADC_SOC);
    SoftwareTriggerSOC(TEMP_BL_ADC_MODULE,    TEMP_BL_ADC_SOC);
    SoftwareTriggerSOC(TEMP_CH_ADC_MODULE,    TEMP_CH_ADC_SOC);
    SoftwareTriggerSOC(TEMP_CL_ADC_MODULE,    TEMP_CL_ADC_SOC);

}

void TriggerVoltageADC(void)
{
    SoftwareTriggerSOC(V_A_ADC_MODULE,    V_A_ADC_SOC);
    SoftwareTriggerSOC(V_B_ADC_MODULE,    V_B_ADC_SOC);
    SoftwareTriggerSOC(V_C_ADC_MODULE,    V_C_ADC_SOC);
    SoftwareTriggerSOC(V_DC_ADC_MODULE,   V_DC_ADC_SOC);
}

void ADC_TriggerCurrents(void)
{
    SoftwareTriggerSOC(I_A_ADC_MODULE,    I_A_ADC_SOC);
    SoftwareTriggerSOC(I_B_ADC_MODULE,    I_B_ADC_SOC);
    SoftwareTriggerSOC(I_C_ADC_MODULE,    I_C_ADC_SOC);
}

/* -----------------------------------------------------------------------
 * Physical getters
 * ----------------------------------------------------------------------- 
 */

/* --- Temperatures (LUT -> degC) ----------------------------------------- */

float GetTempAH(void)
{
    uint16_t raw = GetADCResult(TEMP_AH_ADC_MODULE, TEMP_AH_ADC_SOC);
    raw = MovingAvgU16_Push(TEMP_MA_WINDOW, &temp_ma[0], raw);
    adc.tempAH = raw_to_celsius(raw, temp_lut, TEMP_LUT_LEN);
    return adc.tempAH;
}

float GetTempAL(void)
{
    uint16_t raw = GetADCResult(TEMP_AL_ADC_MODULE, TEMP_AL_ADC_SOC);
    raw = MovingAvgU16_Push(TEMP_MA_WINDOW, &temp_ma[1], raw);
    adc.tempAL = raw_to_celsius(raw, temp_lut, TEMP_LUT_LEN);
    return adc.tempAL;
}

float GetTempBH(void)
{
    uint16_t raw = GetADCResult(TEMP_BH_ADC_MODULE, TEMP_BH_ADC_SOC);
    raw = MovingAvgU16_Push(TEMP_MA_WINDOW, &temp_ma[2], raw);
    adc.tempBH = raw_to_celsius(raw, temp_lut, TEMP_LUT_LEN);
    return adc.tempBH;
}

float GetTempBL(void)
{
    uint16_t raw = GetADCResult(TEMP_BL_ADC_MODULE, TEMP_BL_ADC_SOC);
    raw = MovingAvgU16_Push(TEMP_MA_WINDOW, &temp_ma[3], raw);
    adc.tempBL = raw_to_celsius(raw, temp_lut, TEMP_LUT_LEN);
    return adc.tempBL;
}

float GetTempCH(void)
{
    uint16_t raw = GetADCResult(TEMP_CH_ADC_MODULE, TEMP_CH_ADC_SOC);
    raw = MovingAvgU16_Push(TEMP_MA_WINDOW, &temp_ma[4], raw);
    adc.tempCH = raw_to_celsius(raw, temp_lut, TEMP_LUT_LEN);
    return adc.tempCH;
}

float GetTempCL(void)
{
    uint16_t raw = GetADCResult(TEMP_CL_ADC_MODULE, TEMP_CL_ADC_SOC);
    raw = MovingAvgU16_Push(TEMP_MA_WINDOW, &temp_ma[5], raw);
    adc.tempCL = raw_to_celsius(raw, temp_lut, TEMP_LUT_LEN);
    return adc.tempCL;
}

/* --- Voltages (gain + offset) ----------------------------------------- */

float GetVoltageA(void)
{
    uint16_t raw = GetADCResult(V_A_ADC_MODULE, V_A_ADC_SOC);
    raw = MovingAvgU16_Push(VOLT_MA_WINDOW, &volt_ma[0], raw);
    adc.voltageA = ((float)raw + VOLTAGE_OFFSET) * VOLTAGE_GAIN;
    return adc.voltageA;
}

float GetVoltageB(void)
{
    uint16_t raw = GetADCResult(V_B_ADC_MODULE, V_B_ADC_SOC);
    raw = MovingAvgU16_Push(VOLT_MA_WINDOW, &volt_ma[1], raw);
    adc.voltageB = ((float)raw + VOLTAGE_OFFSET) * VOLTAGE_GAIN;
    return adc.voltageB;
}

float GetVoltageC(void)
{
    uint16_t raw = GetADCResult(V_C_ADC_MODULE, V_C_ADC_SOC);
    raw = MovingAvgU16_Push(VOLT_MA_WINDOW, &volt_ma[2], raw);
    adc.voltageC = ((float)raw + VOLTAGE_OFFSET) * VOLTAGE_GAIN;
    return adc.voltageC;
}

float GetVoltageDC(void)
{
    uint16_t raw = GetADCResult(V_DC_ADC_MODULE, V_DC_ADC_SOC);
    raw = MovingAvgU16_Push(VOLT_MA_WINDOW, &volt_ma[3], raw);
    adc.voltageDC = ((float)raw + VOLTAGE_OFFSET) * VOLTAGE_GAIN;
    return adc.voltageDC;
}

/* --- Currents (gain + offset) ----------------------------------------- */

float GetCurrentA(void)
{
    uint16_t raw = GetADCResult(I_A_ADC_MODULE, I_A_ADC_SOC);
    return ((float)raw +  current_offsets[0]) * CURRENT_GAIN;
}

float GetCurrentB(void)
{
    uint16_t raw = GetADCResult(I_B_ADC_MODULE, I_B_ADC_SOC);
    return ((float)raw +  current_offsets[1]) * CURRENT_GAIN;
}

float GetCurrentC(void)
{
    uint16_t raw = GetADCResult(I_C_ADC_MODULE, I_C_ADC_SOC);
    return ((float)raw +  current_offsets[2]) * CURRENT_GAIN;
}
