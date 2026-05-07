#ifndef __RECORDER_DEFINES_H_
#define __RECORDER_DEFINES_H_

/* -------- PLEASE FILL IN THE FIELDS BELOW -------- */

// TUTORIAL: https://docs.mcuviewer.com/documentation/Recorder/Recorder.html#recorder
// EXAMPLE: https://docs.mcuviewer.com/documentation/Examples/Examples.html#examples

#define ____RECORDER_TIMEBASE_NS   (100000)	// Time period between two consecutive recorderStep() calls in nanoseconds.
#define ____RECORDER_BUFFERSIZE	   (15000)	// Size of recorder buffer - the more samples the longer recording can be done.
#define ____RECORDER_MAXVARS	   (10)		// Limit of recorder variables. Can be used as limiter if recorder overhead is too high for higher number of variables.
#define ____RECORDER_FLOAT_SUPPORT (1)		// Float support enable/disable. On targets that do not have FPU it is advised to disable it.

#define ____RECORDER_C2000_SUPPORT (1)      // C2000 support enable/disable

/* ------------------------------------------------- */

#endif
