/**
 * @file batch.h
 * @brief Tests in batch
 * @author David Redondo
 * @date 2026
 */

#ifndef __BATCH_H__
#define __BATCH_H__


// Default test parameters
#define VOLTAGE_C             185.0F   // Peak voltage (Vpk)
#define DEADTIME_C            1000U    // Dead time (ns)
#define FREQUENCY_C           10000U   // Switching frequency (Hz)
#define FUNDAMENTAL_FREQUENCY 50.0F    // Fundamental frequency (Hz)
#define DELAY_BETWEEN_TESTS   10000U   // Settling delay between steps (ms)

// Array sentinel values
#define END_U32    0xFFFFFFFFu  // Sentinel for uint32_t arrays
#define END_FLOAT  -1.0f        // Sentinel for float arrays

// Batch dimension limits
#define MODE_COUNT      2U  // Number of test modes
#define STEP_COUNT      4U  // Number of current step types
#define MAX_FREQS       5U  // Maximum number of frequencies in a batch
#define MAX_DEADTIMES   5U  // Maximum number of dead times in a batch
#define MAX_CURRENTS    5U  // Maximum number of current levels in a batch

// @brief Converter operating mode
typedef enum {
    ModeSingle      = 0,  // Single converter operation
    ModeInterleaved = 1   // Interleaved converter operation
} TestModeTypeDef;

// @brief Current step direction and axis
typedef enum {
    StepIdPos = 0,  // Positive d-axis step
    StepIdNeg = 1,  // Negative d-axis step
    StepIqPos = 2,  // Positive q-axis step
    StepIqNeg = 3   // Negative q-axis step
} CurrentStepTypeDef;

// @brief Temperature readings from a single batch sample 
typedef struct {
    uint16_t temp_ah;  // Phase A high-side temperature (degC * 10)
    uint16_t temp_al;  // Phase A low-side temperature (degC * 10)
    uint16_t temp_bh;  // Phase B high-side temperature (degC * 10)
    uint16_t temp_bl;  // Phase B low-side temperature (degC * 10)
} BatchSampleTypeDef;

// @brief Internal state machine states for the batch runner
typedef enum {
    BatchIdle  = 0,  // Not running, waiting for StartBatch()
    BatchSetup = 1,  // One-time hardware init on first call after start
    BatchApply = 2,  // Apply settings for current index combo, start timer
    BatchWait  = 3,  // Wait for settling delay, then capture and advance
    BatchDone  = 4   // Disable hardware, return to idle
} BatchStateTypeDef;

//@brief Arms the batch runner. Call once before the scheduler starts ticking RunTests().
//@return none
void StartBatch(void);

//@brief Non-blocking batch tick. Call from the scheduler on every cycle.
//       Internally drives the state machine: applies settings, waits for
//       DELAY_BETWEEN_TESTS ms, captures a sample, then advances to the
//       next index combination. Returns immediately on every call.
//@return none
void RunTests(void);

//@brief Returns 1 if the last batch run has completed, 0 otherwise.
//       Resets to 0 as soon as StartBatch() is called again.
//@return uint32_t - 1 if done, 0 if idle or running
uint32_t IsBatchComplete(void);

#endif // __BATCH_H__
