#ifndef __MAX9814_H
#define __MAX9814_H
#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

// 采样率（软件模拟，先验证硬件）
#define MAX9814_FRAME_SIZE 256

// 初始化MAX9814（内部调用你的ADC_Init）
void max9814_init(void);
// 采集一帧数据（软件轮询，256点）
void max9814_record_frame(uint16_t *raw_buf, int16_t *pcm_buf);

#endif