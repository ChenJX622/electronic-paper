#ifndef __DSP_PROCESS_H
#define __DSP_PROCESS_H
#include "hal_data.h"
#include <stdint.h>
#include "arm_math.h"

// 初始化CMSIS-DSP
void dsp_process_init(void);
// 处理音频数据（返回处理后的16位PCM指针）
int16_t* dsp_process_audio(uint16_t *raw_adc_buf);

#endif