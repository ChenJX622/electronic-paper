#include "max9814.h"
#include "ADC_MY.h"

void max9814_init(void)
{
    ADC_Init();
}

void max9814_record_frame(uint16_t *raw_buf, int16_t *pcm_buf)
{
    for(uint16_t i = 0; i < MAX9814_FRAME_SIZE; i++)
    {
        raw_buf[i] = ADC_Read_Raw();
        int32_t pcm = (int32_t)raw_buf[i] - 2048;
        pcm_buf[i] = (int16_t)(pcm * 16);
        for(volatile uint32_t j = 0; j < 500; j++);
    }
}