/**
 * @file batch.c
 * @brief Tests in batch
 * @author David Redondo
 * @date 2026
 */

#include <stdint.h>
#include <stddef.h>
#include "bsp_epwm.h"
#include "batch.h"
#include "adc_config.h"
#include "control_loop.h"

// Frequency sweep values (Hz) — editable at runtime via watch window
static uint32_t frequencies[MAX_FREQS + 1] = {
    4000U,
    8000U,
    12000U,
    16000U,
    20000U,
    END_U32
};

// Dead time sweep values (ns) — editable at runtime via watch window
static uint32_t deadtimes[MAX_DEADTIMES + 1] = {
    2000U,
    4000U,
    6000U,
    8000U,
    10000U,
    END_U32
};

// Current sweep values (A) — editable at runtime via watch window
static float currents[MAX_CURRENTS + 1] = {
    4.0F,
    8.0F,
    12.0F,
    16.0F,
    20.0F,
    END_FLOAT
};

// Results matrix: mode -> frequency -> deadtime -> current -> step
static volatile BatchSampleTypeDef results[MODE_COUNT]
                                          [MAX_FREQS]
                                          [MAX_DEADTIMES]
                                          [MAX_CURRENTS]
                                          [STEP_COUNT];

// State machine position and timer
static BatchStateTypeDef s_state        = BatchIdle;
static uint32_t          s_mode         = 0U;
static uint32_t          s_fi           = 0U;
static uint32_t          s_di           = 0U;
static uint32_t          s_ci           = 0U;
static uint32_t          s_step         = 0U;
static uint32_t          s_timer        = 0U;
static uint32_t          s_batch_done   = 0U; // flag polled by IsBatchComplete()

//@brief Captures a temperature sample from all four MOSFET positions
//@return BatchSampleTypeDef populated with current ADC readings
static BatchSampleTypeDef CaptureData(void)
{
    BatchSampleTypeDef sample;

    sample.temp_ah = GetTempAH();
    sample.temp_al = GetTempAL();
    sample.temp_bh = GetTempBH();
    sample.temp_bl = GetTempBL();

    return sample;
}

//@brief Captures and stores a sample into the results matrix
//@param [in] mode  Operating mode index
//@param [in] fi    Frequency index
//@param [in] di    Dead time index
//@param [in] ci    Current index
//@param [in] step  Current step (axis and direction)
static void SaveSample(TestModeTypeDef mode,
                       uint32_t fi,
                       uint32_t di,
                       uint32_t ci,
                       CurrentStepTypeDef step)
{
    results[mode][fi][di][ci][step] = CaptureData();
}

//@brief Applies the Id/Iq reference for the current step index
//@param [in] current  Reference current magnitude (A)
//@param [in] step     Current step to apply
static void ApplyCurrentStep(float current, CurrentStepTypeDef step)
{
    switch (step)
    {
        case StepIdPos: ControlLoop_SetIdRef( current); ControlLoop_SetIqRef(0.0F);     break;
        case StepIdNeg: ControlLoop_SetIdRef(-current); ControlLoop_SetIqRef(0.0F);     break;
        case StepIqPos: ControlLoop_SetIdRef(0.0F);     ControlLoop_SetIqRef( current); break;
        case StepIqNeg: ControlLoop_SetIdRef(0.0F);     ControlLoop_SetIqRef(-current); break;
        default:                                                  break;
    }
}

//@brief Configures PWM channels for the given operating mode.
//       Called once per mode transition.
//@param [in] mode  Mode to configure
static void ApplyModeSetup(TestModeTypeDef mode)
{
    if (mode == ModeSingle)
    {
        DisablePWM(CHANNEL_B);
        EnablePWM(CHANNEL_A);
    }
    else
    {
        EnablePWM(CHANNEL_A);
        EnablePWM(CHANNEL_B);
        SetPhaseShift(CHANNEL_A, CHANNEL_B, 0.5F);   // 180 deg
    }
}

//@brief Advances the test matrix indices by one step. Wraps inner indices
//       and increments outer ones as needed. Sets s_state to kBatchDone
//       when the last combination has been processed.
static void AdvanceIndices(void)
{
    s_step++;
    if (s_step < STEP_COUNT)
    {
        return;
    }
    s_step = 0U;

    s_ci++;
    if (s_ci < MAX_CURRENTS && currents[s_ci] != END_FLOAT)
    {
        return;
    }
    s_ci = 0U;

    s_di++;
    if (s_di < MAX_DEADTIMES && deadtimes[s_di] != END_U32)
    {
        return;
    }
    s_di = 0U;

    s_fi++;
    if (s_fi < MAX_FREQS && frequencies[s_fi] != END_U32)
    {
        return;
    }
    s_fi = 0U;

    s_mode++;
    if (s_mode < MODE_COUNT)
    {
        return;
    }

    // All combinations exhausted
    s_state = BatchDone;
}

void StartBatch(void)
{
    s_mode       = 0U;
    s_fi         = 0U;
    s_di         = 0U;
    s_ci         = 0U;
    s_step       = 0U;
    s_batch_done = 0U;
    s_state      = BatchSetup;
}

uint32_t IsBatchComplete(void)
{
    return s_batch_done;
}

void RunTests(void)
{
    switch (s_state)
    {
        case BatchIdle:
            break;

        case BatchSetup:
            EnablePWM(CHANNEL_C);
            SetFrequency(CHANNEL_C, FREQUENCY_C);
            SetDeadTime(CHANNEL_C, DEADTIME_C);
            SetOpenLoopVoltage(VOLTAGE_C, FUNDAMENTAL_FREQUENCY);
            ControlLoop_Enable();
            ControlLoop_SetIdRef(0.0F);
            ControlLoop_SetIqRef(0.0F);
            s_state = BatchApply;
            break;

        case BatchApply:
            // Reconfigure channels at the start of each new mode
            if (s_fi == 0U && s_di == 0U && s_ci == 0U && s_step == 0U)
            {
                ApplyModeSetup((TestModeTypeDef)s_mode);
            }

            SetFrequency(CHANNEL_A, frequencies[s_fi]);
            SetFrequency(CHANNEL_B, frequencies[s_fi]);
            SetDeadTime(CHANNEL_A, deadtimes[s_di]);
            SetDeadTime(CHANNEL_B, deadtimes[s_di]);
            ApplyCurrentStep(currents[s_ci], (CurrentStepTypeDef)s_step);

            StartTimer(&s_timer);
            s_state = BatchWait;
            break;

        case BatchWait:
            if (EvalTimer(s_timer) >= DELAY_BETWEEN_TESTS)
            {
                SaveSample((TestModeTypeDef)s_mode,
                           s_fi, s_di, s_ci,
                           (CurrentStepTypeDef)s_step);

                AdvanceIndices();   // may transition s_state to kBatchDone

                if (s_state != BatchDone)
                {
                    s_state = BatchApply;
                }
            }
            break;

        case BatchDone:
            ControlLoop_SetIdRef(0.0F);
            ControlLoop_SetIqRef(0.0F);
            ControlLoop_Disable();
            DisablePWM(CHANNEL_A);
            DisablePWM(CHANNEL_B);
            DisablePWM(CHANNEL_C);
            s_batch_done = 1U;
            s_state      = BatchIdle;
            break;
    }
}
