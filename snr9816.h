#ifndef __SNR9816_H
#define __SNR9816_H
#include "hal_data.h"
#include <stdint.h>

// 模块核心协议定义
#define TTS_FRAME_HEAD      0xFD    // 固定帧头
#define TTS_CMD_PLAY        0x01    // 合成播放命令
#define TTS_CMD_QUERY_STATE 0x21    // 查询状态命令
#define TTS_CMD_STOP        0x02    // 停止合成
#define TTS_CMD_PAUSE       0x03    // 暂停合成
#define TTS_CMD_RESUME      0x04    // 继续合成

// 编码格式定义
#define TTS_ENCODING_GB2312 0x01
#define TTS_ENCODING_UTF8   0x04    // 原生UTF-8支持，无需转码

// 模块状态返回
#define TTS_STATE_IDLE      0x4F    // 空闲状态
#define TTS_STATE_BUSY      0x4E    // 忙状态
#define TTS_FRAME_ACK       0x41    // 命令帧接收正确

// 参数范围 0-9
#define TTS_VOLUME_MIN      0
#define TTS_VOLUME_MAX      9
#define TTS_SPEED_MIN       0
#define TTS_SPEED_MAX       9
#define TTS_TONE_MIN        0
#define TTS_TONE_MAX        9

// 发音人定义
#define TTS_VOICE_FEMALE    "[m0]"  // 女声（默认）
#define TTS_VOICE_MALE      "[m1]"  // 男声

// 初始化TTS模块（对应UART3，P707/TX、P706/RX）
fsp_err_t snr9816_tts_init(void);

// 查询模块状态：返回0=空闲，1=忙，<0=通信异常
int snr9816_tts_query_state(void);

// 播报UTF-8文本（直接传入ESP32返回的UTF-8字符串，无需转码）
fsp_err_t snr9816_tts_speak_utf8(uint8_t *utf8_text, uint16_t text_len);

// 播报GB2312文本
fsp_err_t snr9816_tts_speak_gb2312(uint8_t *gb2312_text, uint16_t text_len);

// 音量/语速/语调设置 0-9
fsp_err_t snr9816_tts_set_volume(uint8_t volume);
fsp_err_t snr9816_tts_set_speed(uint8_t speed);
fsp_err_t snr9816_tts_set_tone(uint8_t tone);

// 发音人切换
fsp_err_t snr9816_tts_set_voice(const char *voice_code);

// 播放控制
fsp_err_t snr9816_tts_stop(void);
fsp_err_t snr9816_tts_pause(void);
fsp_err_t snr9816_tts_resume(void);

#endif