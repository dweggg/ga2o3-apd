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
#include "task_scheduler.h"

// Frequency sweep values (Hz) - editable at runtime via watch window
static uint32_t frequencies[MAX_FREQS + 1] = {
    4000U,
    8000U,
    12000U,
    16000U,
    20000U,
    END_U32
};

// Dead time sweep values (ns) - editable at runtime via watch window
static uint32_t deadtimes[MAX_DEADTIMES + 1] = {
    2000U,
    4000U,
    6000U,
    8000U,
    10000U,
    END_U32
};

// Current sweep values (A) - editable at runtime via watch window
static float currents[MAX_CURRENTS + 1] = {
    4.0F,
    8.0F,
    12.0F,
    16.0F,
    20.0F,
    END_FLOAT
};

// Results matrix: mode -> frequency -> deadtime -> current -> step
// stored in a section of RAM of its own
#pragma DATA_SECTION(results, "batch_results")
static volatile BatchSampleTypeDef results[MODE_COUNT]
                                          [MAX_FREQS]
                                          [MAX_DEADTIMES]
                                          [MAX_CURRENTS]
                                          [STEP_COUNT];
// State machine position and timer
static BatchStateTypeDef batch_state        = BatchIdle;
static uint32_t          batch_mode         = 0U;
static uint32_t          batch_fi           = 0U;
static uint32_t          batch_di           = 0U;
static uint32_t          batch_ci           = 0U;
static uint32_t          batch_step         = 0U;
static uint32_t          batch_timer        = 0U;
static uint32_t          batch_batch_done   = 0U; // flag polled by IsBatchComplete()

//@brief Captures a temperature sample from all four MOSFET positions
//@return BatchSampleTypeDef populated with temperature ADC readings (celsius x10)
static BatchSampleTypeDef CaptureData(void)
{
    BatchSampleTypeDef sample;

    sample.temp_ah = (uint16_t)(GetTempAH() * 10.0f);
    sample.temp_al = (uint16_t)(GetTempAL() * 10.0f);
    sample.temp_bh = (uint16_t)(GetTempBH() * 10.0f);
    sample.temp_bl = (uint16_t)(GetTempBL() * 10.0f);

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
        DisablePWM(PWM_CHANNEL_B);
        EnablePWM(PWM_CHANNEL_A);
        ControlLoop_SetInterleavedMode(0);
    }
    else
    {
        EnablePWM(PWM_CHANNEL_A);
        EnablePWM(PWM_CHANNEL_B);
        ControlLoop_SetInterleavedMode(1);
    }
}

//@brief Advances the test matrix indices by one step. Wraps inner indices
//       and increments outer ones as needed. Sets batch_state to kBatchDone
//       when the last combination has been processed.
static void AdvanceIndices(void)
{
    batch_step++;
    if (batch_step < STEP_COUNT)
    {
        return;
    }
    batch_step = 0U;

    batch_ci++;
    if (batch_ci < MAX_CURRENTS && currents[batch_ci] != END_FLOAT)
    {
        return;
    }
    batch_ci = 0U;

    batch_di++;
    if (batch_di < MAX_DEADTIMES && deadtimes[batch_di] != END_U32)
    {
        return;
    }
    batch_di = 0U;

    batch_fi++;
    if (batch_fi < MAX_FREQS && frequencies[batch_fi] != END_U32)
    {
        return;
    }
    batch_fi = 0U;

    batch_mode++;
    if (batch_mode < MODE_COUNT)
    {
        return;
    }

    // All combinations exhausted
    batch_state = BatchDone;
}

void StartBatch(void)
{
    batch_mode       = 0U;
    batch_fi         = 0U;
    batch_di         = 0U;
    batch_ci         = 0U;
    batch_step       = 0U;
    batch_batch_done = 0U;
    batch_state      = BatchSetup;
}

uint32_t IsBatchComplete(void)
{
    return batch_batch_done;
}

void RunTests(void)
{
    switch (batch_state)
    {
        case BatchIdle:
            break;

        case BatchSetup:
            EnablePWM(PWM_CHANNEL_C);
            SetFrequency(PWM_CHANNEL_C, FREQUENCY_C);
            SetDeadTime(PWM_CHANNEL_C, DEADTIME_C);
            ControlLoop_SetOpenLoopVoltage(VOLTAGE_C, FUNDAMENTAL_FREQUENCY);
            ControlLoop_Enable();
            ControlLoop_SetIdRef(0.0F);
            ControlLoop_SetIqRef(0.0F);
            batch_state = BatchApply;
            break;

        case BatchApply:
            // Reconfigure channels at the start of each new mode
            if (batch_fi == 0U && batch_di == 0U && batch_ci == 0U && batch_step == 0U)
            {
                ApplyModeSetup((TestModeTypeDef)batch_mode);
            }

            SetFrequency(PWM_CHANNEL_A, frequencies[batch_fi]);
            SetFrequency(PWM_CHANNEL_B, frequencies[batch_fi]);
            SetDeadTime(PWM_CHANNEL_A, deadtimes[batch_di]);
            SetDeadTime(PWM_CHANNEL_B, deadtimes[batch_di]);
            ApplyCurrentStep(currents[batch_ci], (CurrentStepTypeDef)batch_step);

            StartTimer(&batch_timer);
            batch_state = BatchWait;
            break;

        case BatchWait:
            if (EvalTimer(batch_timer) >= DELAY_BETWEEN_TESTS)
            {
                SaveSample((TestModeTypeDef)batch_mode,
                           batch_fi, batch_di, batch_ci,
                           (CurrentStepTypeDef)batch_step);

                AdvanceIndices();   // may transition batch_state to kBatchDone

                if (batch_state != BatchDone)
                {
                    batch_state = BatchApply;
                }
            }
            break;

        case BatchDone:
            ControlLoop_SetIdRef(0.0F);
            ControlLoop_SetIqRef(0.0F);
            ControlLoop_Disable();
            DisablePWM(PWM_CHANNEL_A);
            DisablePWM(PWM_CHANNEL_B);
            DisablePWM(PWM_CHANNEL_C);
            batch_batch_done = 1U;
            batch_state      = BatchIdle;
            break;
    }
}
