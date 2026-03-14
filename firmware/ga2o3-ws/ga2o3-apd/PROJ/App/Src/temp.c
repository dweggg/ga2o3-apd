#include "temp.h"


typedef struct
{
    IGBTPositionTypeDef position;
    uint16_t currently_in_use;
    float32_t actual_temp;
} IGBTSingleTempTypeDef;

typedef struct
{
    uint16_t adc_raw[6];
    IGBTSingleTempTypeDef igbt_temp_array[6];
};
