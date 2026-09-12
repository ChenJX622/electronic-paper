#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// 音频核心参数（与ESP32端完全对齐）
#define AUDIO_SAMPLE_RATE       16000       // 16kHz采样率
#define AUDIO_FRAME_SAMPLES     320         // 20ms一帧（16000×0.02=320采样点）
#define AUDIO_BUFFER_SIZE       3200        // 环形缓存（最多10帧）
#define NOISE_THRESHOLD         100         // 噪声过滤阈值

// 瑞萨→ESP32 音频帧协议（与ESP32端接收逻辑完全一致）
#define FRAME_HEAD_1            0xAA        // 帧头1
#define FRAME_HEAD_2            0x55        // 帧头2
#define FRAME_TYPE_AUDIO        0x01        // 帧类型：音频数据
#define FRAME_TAIL              0xEE        // 帧尾

// ESP32→瑞萨 识别结果帧协议（与ESP32端发送格式对齐）
#define RESULT_FRAME_HEAD1      0xBB        // 结果帧头1
#define RESULT_FRAME_HEAD2      0x66        // 结果帧头2
#define RESULT_FRAME_TAIL1       0x66        // 结果帧尾1
#define RESULT_FRAME_TAIL2       0xBB        // 结果帧尾2

// 接收缓冲区与结果长度限制
#define RECV_BUFFER_SIZE        256         // 串口接收环形缓冲区
#define RESULT_MAX_LEN          256         // 识别结果最大长度

// 超时发送时间（500ms未攒满帧也强制发送）
#define SEND_TIMEOUT_MS         500

// 外部变量（供hal_entry调用）
extern int16_t audio_sample_buffer[AUDIO_BUFFER_SIZE];
extern uint32_t audio_write_idx;

// 音频模块初始化（UART8 + ADC + GPT定时器）
fsp_err_t audio_init(void);

// 启动MAX9814采集
fsp_err_t audio_collect_start(void);

// 停止采集
fsp_err_t audio_collect_stop(void);

// 获取单采样点16位PCM数据
fsp_err_t audio_sample_get(int16_t *sample);

// 噪声过滤：返回true=有效语音
bool audio_noise_filter(int16_t sample);

// 打包音频帧并发送到ESP32
fsp_err_t audio_frame_send(int16_t *samples, uint16_t sample_num);

// 获取ESP32返回的识别结果
uint8_t audio_asr_result_get(uint8_t *out_buf, uint32_t max_len);

// 音频处理主函数（循环调用，处理超时发送+结果解析）
void audio_process(void);

#endif // __AUDIO_H__