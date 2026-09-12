#include "snr9816.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TTS_UART g_uart3
volatile bool tts_tx_done = false;
volatile bool tts_rx_done = false;
uint8_t tts_rx_byte = 0;

// UART3回调函数，必须在RASC里配置UART3的回调为这个函数
void uart3_callback(uart_callback_args_t *p_args)
{
    if(UART_EVENT_TX_COMPLETE == p_args->event)
    {
        tts_tx_done = true;
    }
    else if(UART_EVENT_RX_CHAR == p_args->event)
    {
        tts_rx_byte = (uint8_t)p_args->data;
        tts_rx_done = true;
    }
}

// 模块初始化
fsp_err_t snr9816_tts_init(void)
{
    fsp_err_t err = TTS_UART.p_api->open(TTS_UART.p_ctrl, TTS_UART.p_cfg);
    if(FSP_SUCCESS != err) return err;
    
    // 等待模块上电就绪
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    return FSP_SUCCESS;
}

// 查询模块工作状态
int snr9816_tts_query_state(void)
{
    // 标准查询命令：0xFD 0x00 0x01 0x21
    uint8_t cmd[4] = {TTS_FRAME_HEAD, 0x00, 0x01, TTS_CMD_QUERY_STATE};
    tts_tx_done = false;
    tts_rx_done = false;
    
    fsp_err_t err = TTS_UART.p_api->write(TTS_UART.p_ctrl, cmd, sizeof(cmd));
    if(FSP_SUCCESS != err) return -1;
    
    // 等待发送完成
    volatile uint32_t timeout = 100000;
    while(!tts_tx_done && timeout-- > 0);
    if(!tts_tx_done) return -2;
    
    // 等待模块返回状态
    timeout = 1000000;
    while(!tts_rx_done && timeout-- > 0);
    if(!tts_rx_done) return -3;
    
    if(tts_rx_byte == TTS_STATE_IDLE) return 0;
    if(tts_rx_byte == TTS_STATE_BUSY) return 1;
    return -4;
}

// 底层通用发送合成帧函数
static fsp_err_t tts_send_frame(uint8_t encoding, uint8_t *text, uint16_t text_len)
{
    if(text == NULL || text_len == 0) return FSP_ERR_INVALID_ARGUMENT;
    
    // 等待模块空闲，最大等待2秒
    int state;
    uint32_t retry = 200;
    do {
        state = snr9816_tts_query_state();
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
        retry--;
    } while(state == 1 && retry > 0);
    
    if(state != 0) return -1;
    
    // 计算帧长度（完全匹配手册定义，无校验和）
    uint16_t data_len = 1 + 1 + text_len; // 命令字(1) + 编码参数(1) + 文本长度
    uint16_t frame_len = 1 + 2 + 1 + 1 + text_len; // 帧头(1) + 长度(2) + 命令(1) + 编码(1) + 文本
    uint8_t *cmd = (uint8_t*)malloc(frame_len);
    if(cmd == NULL) return FSP_ERR_OUT_OF_MEMORY;
    
    // 填充帧结构
    cmd[0] = TTS_FRAME_HEAD;
    cmd[1] = (data_len >> 8) & 0xFF; // 长度高8位
    cmd[2] = data_len & 0xFF;        // 长度低8位
    cmd[3] = TTS_CMD_PLAY;
    cmd[4] = encoding;
    memcpy(&cmd[5], text, text_len);
    
    // 发送帧
    tts_tx_done = false;
    fsp_err_t err = TTS_UART.p_api->write(TTS_UART.p_ctrl, cmd, frame_len);
    if(FSP_SUCCESS != err)
    {
        free(cmd);
        return err;
    }
    
    // 等待发送完成
    volatile uint32_t timeout = 1000000;
    while(!tts_tx_done && timeout-- > 0);
    free(cmd);
    
    return FSP_SUCCESS;
}

// 播报UTF-8文本（核心函数，直接用ESP32返回的UTF-8文本）
fsp_err_t snr9816_tts_speak_utf8(uint8_t *utf8_text, uint16_t text_len)
{
    return tts_send_frame(TTS_ENCODING_UTF8, utf8_text, text_len);
}

// 播报GB2312文本
fsp_err_t snr9816_tts_speak_gb2312(uint8_t *gb2312_text, uint16_t text_len)
{
    return tts_send_frame(TTS_ENCODING_GB2312, gb2312_text, text_len);
}

// 设置音量 0-9
fsp_err_t snr9816_tts_set_volume(uint8_t volume)
{
    if(volume > TTS_VOLUME_MAX) volume = TTS_VOLUME_MAX;
    char vol_text[8] = {0};
    sprintf(vol_text, "[v%d]", volume);
    return tts_send_frame(TTS_ENCODING_UTF8, (uint8_t*)vol_text, strlen(vol_text));
}

// 设置语速 0-9
fsp_err_t snr9816_tts_set_speed(uint8_t speed)
{
    if(speed > TTS_SPEED_MAX) speed = TTS_SPEED_MAX;
    char speed_text[8] = {0};
    sprintf(speed_text, "[s%d]", speed);
    return tts_send_frame(TTS_ENCODING_UTF8, (uint8_t*)speed_text, strlen(speed_text));
}

// 设置语调 0-9
fsp_err_t snr9816_tts_set_tone(uint8_t tone)
{
    if(tone > TTS_TONE_MAX) tone = TTS_TONE_MAX;
    char tone_text[8] = {0};
    sprintf(tone_text, "[t%d]", tone);
    return tts_send_frame(TTS_ENCODING_UTF8, (uint8_t*)tone_text, strlen(tone_text));
}

// 设置发音人
fsp_err_t snr9816_tts_set_voice(const char *voice_code)
{
    if(voice_code == NULL) return FSP_ERR_INVALID_ARGUMENT;
    return tts_send_frame(TTS_ENCODING_UTF8, (uint8_t*)voice_code, strlen(voice_code));
}

// 停止播放
fsp_err_t snr9816_tts_stop(void)
{
    uint8_t cmd[4] = {TTS_FRAME_HEAD, 0x00, 0x01, TTS_CMD_STOP};
    tts_tx_done = false;
    fsp_err_t err = TTS_UART.p_api->write(TTS_UART.p_ctrl, cmd, sizeof(cmd));
    if(FSP_SUCCESS != err) return err;
    
    volatile uint32_t timeout = 100000;
    while(!tts_tx_done && timeout-- > 0);
    return FSP_SUCCESS;
}

// 暂停播放
fsp_err_t snr9816_tts_pause(void)
{
    uint8_t cmd[4] = {TTS_FRAME_HEAD, 0x00, 0x01, TTS_CMD_PAUSE};
    tts_tx_done = false;
    fsp_err_t err = TTS_UART.p_api->write(TTS_UART.p_ctrl, cmd, sizeof(cmd));
    if(FSP_SUCCESS != err) return err;
    
    volatile uint32_t timeout = 100000;
    while(!tts_tx_done && timeout-- > 0);
    return FSP_SUCCESS;
}

// 继续播放
fsp_err_t snr9816_tts_resume(void)
{
    uint8_t cmd[4] = {TTS_FRAME_HEAD, 0x00, 0x01, TTS_CMD_RESUME};
    tts_tx_done = false;
    fsp_err_t err = TTS_UART.p_api->write(TTS_UART.p_ctrl, cmd, sizeof(cmd));
    if(FSP_SUCCESS != err) return err;
    
    volatile uint32_t timeout = 100000;
    while(!tts_tx_done && timeout-- > 0);
    return FSP_SUCCESS;
}