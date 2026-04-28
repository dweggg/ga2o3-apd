/**
 * @file batch.c
 * @brief Batch testing state machine: sweeps through test parameters and captures data
 * @author David Redondo
 * @date 2026
 *
 * This module manages the batch test sequence only. Hardware configuration
 * (PWM enable/disable, frequencies, dead times) is applied by the UI layer.
 * The control loop and other system components are configured through their
 * public interfaces, not directly manipulated here.
 */

#include <stdint.h>
#include <stddef.h>
#include "batch.h"
#include "control_loop.h"
#include "adc_config.h"
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

//@brief Query the current test mode index
//@return Current mode being tested
uint32_t BatchGetCurrentMode(void)
{
    return batch_mode;
}

//@brief Query the current frequency index
//@return Current frequency index
uint32_t BatchGetCurrentFrequencyIndex(void)
{
    return batch_fi;
}

//@brief Query the current deadtime index
//@return Current deadtime index
uint32_t BatchGetCurrentDeadtimeIndex(void)
{
    return batch_di;
}

//@brief Query the current frequency value
//@return Current frequency in Hz
uint32_t BatchGetCurrentFrequency(void)
{
    return frequencies[batch_fi];
}

//@brief Query the current deadtime value
//@return Current deadtime in ns
uint32_t BatchGetCurrentDeadtime(void)
{
    return deadtimes[batch_di];
}

//@brief Advances the test matrix indices by one step. Wraps inner indices
//       and increments outer ones as needed. Sets batch_state to BatchDone
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

//@brief Batch state machine core. Call every scheduler tick while batch is active.
//       Returns immediately on every call. State transitions happen based on timers
//       and index progression.
void RunTests(void)
{
    switch (batch_state)
    {
        case BatchIdle:
            break;

        case BatchSetup:
            // Initial setup: configure control loop and CH for reference
            ControlLoop_SetOpenLoopVoltage(VOLTAGE_C, FUNDAMENTAL_FREQUENCY);
            ControlLoop_Enable();
            ControlLoop_SetIdRef(0.0F);
            ControlLoop_SetIqRef(0.0F);
            batch_state = BatchApply;
            break;

        case BatchApply:
            // Request UI to apply current frequency/deadtime settings.
            // Mode and interleaving mode changes happen at mode boundaries (via UI query).
            // This state just signals what settings to apply; UI handles the actual calls.
            
            // Apply current test step's current reference
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

                AdvanceIndices();   // may transition batch_state to BatchDone

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
            batch_batch_done = 1U;
            batch_state      = BatchIdle;
            break;
    }
}
