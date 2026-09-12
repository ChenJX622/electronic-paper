#include "audio.h"
#include "sensor_frame.h"

// 静态全局变量（仅本文件访问，避免重复定义冲突）
static volatile bool adc_convert_done = false;
static volatile bool uart_esp_tx_done = false;

// 音频采样环形缓存
int16_t audio_sample_buffer[AUDIO_BUFFER_SIZE] = {0};
uint32_t audio_write_idx = 0;
static uint32_t audio_read_idx = 0;
static uint32_t last_send_time = 0;

// ESP32结果接收缓存与状态机
static uint8_t recv_ring_buf[RECV_BUFFER_SIZE] = {0};
static volatile uint32_t recv_write_idx = 0;
static volatile uint32_t recv_read_idx = 0;
static char result_buf[RESULT_MAX_LEN] = {0};
static volatile bool new_result_flag = false;

// 接收状态机枚举（解析ESP32识别结果帧）
typedef enum {
    RECV_STATE_WAIT_HEAD1,
    RECV_STATE_WAIT_HEAD2,
    RECV_STATE_WAIT_LEN,
    RECV_STATE_WAIT_DATA,
    RECV_STATE_WAIT_TAIL1,
    RECV_STATE_WAIT_TAIL2
} recv_state_t;

// GPT0溢出回调（16kHz触发ADC采样，与RASC绑定）
void gpt0_overflow_callback(timer_callback_args_t * p_args)
{
    (void)p_args;
    g_adc0.p_api->scanStart(g_adc0.p_ctrl);
}

// ADC0采样完成回调（与RASC绑定）
void adc0_callback(adc_callback_args_t * p_args)
{
    (void)p_args;
    if(ADC_EVENT_SCAN_COMPLETE == p_args->event)
    {
        adc_convert_done = true;
    }
}

// UART8回调（收发一体，与RASC绑定，解决重复定义问题）
void uart8_callback(uart_callback_args_t * p_args)
{
    (void)p_args;
    switch(p_args->event)
    {
        case UART_EVENT_TX_COMPLETE:
            uart_esp_tx_done = true;
            break;
        case UART_EVENT_RX_CHAR:
            recv_ring_buf[recv_write_idx++] = (uint8_t)p_args->data;
            if(recv_write_idx >= RECV_BUFFER_SIZE) recv_write_idx = 0;
            break;
        default:
            break;
    }
}

// 音频模块初始化
fsp_err_t audio_init(void)
{
    fsp_err_t err;
    // 初始化UART8（与ESP32通信，115200 8N1）
    err = g_uart8.p_api->open(g_uart8.p_ctrl, g_uart8.p_cfg);
    if(FSP_SUCCESS != err) return err;
    // 初始化ADC0（MAX9814通道AN010）
    err = g_adc0.p_api->open(g_adc0.p_ctrl, g_adc0.p_cfg);
    if(FSP_SUCCESS != err) return err;
    err = g_adc0.p_api->scanCfg(g_adc0.p_ctrl, g_adc0.p_channel_cfg);
    if(FSP_SUCCESS != err) return err;
    // 初始化GPT0（16kHz采样触发）
    err = g_timer0.p_api->open(g_timer0.p_ctrl, g_timer0.p_cfg);
    if(FSP_SUCCESS != err) return err;
    
    last_send_time = get_sys_tick_ms();
    return FSP_SUCCESS;
}

// 启动采集
fsp_err_t audio_collect_start(void)
{
    return g_timer0.p_api->start(g_timer0.p_ctrl);
}

// 停止采集
fsp_err_t audio_collect_stop(void)
{
    return g_timer0.p_api->stop(g_timer0.p_ctrl);
}

// 获取16位PCM采样点
fsp_err_t audio_sample_get(int16_t *sample)
{
    fsp_err_t err;
    uint16_t adc_raw;
    while(!adc_convert_done);
    adc_convert_done = false;
    err = g_adc0.p_api->read(g_adc0.p_ctrl, ADC_CHANNEL_10, &adc_raw);
    if(FSP_SUCCESS != err) return err;
    // 12位ADC转16位有符号PCM（零点2048，放大16倍）
    *sample = (int16_t)((adc_raw - 2048) * 16);
    return FSP_SUCCESS;
}

// 噪声过滤
bool audio_noise_filter(int16_t sample)
{
    return (abs(sample) > NOISE_THRESHOLD);
}

// 发送音频帧到ESP32（帧格式与ESP32接收逻辑完全匹配）
fsp_err_t audio_frame_send(int16_t *samples, uint16_t sample_num)
{
    fsp_err_t err;
    uint8_t send_buf[1024] = {0};
    uint16_t buf_idx = 0;
    uint8_t checksum = 0;

    // 帧头
    send_buf[buf_idx++] = FRAME_HEAD_1;
    send_buf[buf_idx++] = FRAME_HEAD_2;
    // 帧类型
    send_buf[buf_idx++] = FRAME_TYPE_AUDIO;
    // 采样点数（高字节在前）
    send_buf[buf_idx++] = (uint8_t)(sample_num >> 8);
    send_buf[buf_idx++] = (uint8_t)(sample_num & 0xFF);
    // 校验和（前5字节累加）
    checksum = send_buf[0] + send_buf[1] + send_buf[2] + send_buf[3] + send_buf[4];
    send_buf[buf_idx++] = checksum;
    // 音频数据（小端模式：低字节在前）
    for(uint16_t i = 0; i < sample_num; i++)
    {
        send_buf[buf_idx++] = (uint8_t)(samples[i] & 0xFF);
        send_buf[buf_idx++] = (uint8_t)(samples[i] >> 8);
    }
    // 帧尾
    send_buf[buf_idx++] = FRAME_TAIL;

    // 发送
    uart_esp_tx_done = false;
    err = g_uart8.p_api->write(g_uart8.p_ctrl, send_buf, buf_idx);
    if(FSP_SUCCESS != err) return err;
    while(!uart_esp_tx_done);
    last_send_time = get_sys_tick_ms();
    return FSP_SUCCESS;
}

// 解析ESP32识别结果帧
static void parse_esp32_result(void)
{
    static recv_state_t recv_state = RECV_STATE_WAIT_HEAD1;
    static uint8_t data_len = 0;
    static uint32_t data_idx = 0;

    while(recv_read_idx != recv_write_idx)
    {
        uint8_t data = recv_ring_buf[recv_read_idx++];
        if(recv_read_idx >= RECV_BUFFER_SIZE) recv_read_idx = 0;

        switch(recv_state)
        {
            case RECV_STATE_WAIT_HEAD1:
                if(data == RESULT_FRAME_HEAD1) recv_state = RECV_STATE_WAIT_HEAD2;
                break;
            case RECV_STATE_WAIT_HEAD2:
                if(data == RESULT_FRAME_HEAD2) recv_state = RECV_STATE_WAIT_LEN;
                else recv_state = RECV_STATE_WAIT_HEAD1;
                break;
            case RECV_STATE_WAIT_LEN:
                data_len = data;
                recv_state = RECV_STATE_WAIT_DATA;
                data_idx = 0;
                memset(result_buf, 0, RESULT_MAX_LEN);
                break;
            case RECV_STATE_WAIT_DATA:
                if(data_idx < data_len && data_idx < RESULT_MAX_LEN-1)
                    result_buf[data_idx++] = (char)data;
                else if(data == RESULT_FRAME_TAIL1) recv_state = RECV_STATE_WAIT_TAIL2;
                else { recv_state = RECV_STATE_WAIT_HEAD1; data_idx=0; memset(result_buf,0,RESULT_MAX_LEN); }
                break;
            case RECV_STATE_WAIT_TAIL1:
                if(data == RESULT_FRAME_TAIL1) recv_state = RECV_STATE_WAIT_TAIL2;
                else { recv_state = RECV_STATE_WAIT_HEAD1; data_idx=0; memset(result_buf,0,RESULT_MAX_LEN); }
                break;
            case RECV_STATE_WAIT_TAIL2:
                if(data == RESULT_FRAME_TAIL2) { result_buf[data_idx] = '\0'; new_result_flag = true; }
                recv_state = RECV_STATE_WAIT_HEAD1;
                data_len = 0;
                data_idx = 0;
                break;
            default:
                recv_state = RECV_STATE_WAIT_HEAD1;
                break;
        }
    }
}

// 获取识别结果
uint8_t audio_asr_result_get(uint8_t *out_buf, uint32_t max_len)
{
    if(!new_result_flag) return 0;
    uint32_t copy_len = strlen(result_buf);
    if(copy_len > max_len-1) copy_len = max_len-1;
    strncpy((char *)out_buf, result_buf, copy_len);
    out_buf[copy_len] = '\0';
    new_result_flag = false;
    memset(result_buf, 0, RESULT_MAX_LEN);
    return 1;
}

// 音频处理主函数（超时发送+结果解析）
void audio_process(void)
{
    uint32_t now = get_sys_tick_ms();
    uint32_t available = (audio_write_idx - audio_read_idx + AUDIO_BUFFER_SIZE) % AUDIO_BUFFER_SIZE;

    // 超时发送：500ms未攒满帧也发送
    if(available > 0 && (now - last_send_time > SEND_TIMEOUT_MS))
    {
        audio_frame_send(&audio_sample_buffer[audio_read_idx], (uint16_t)available);
        audio_read_idx = (audio_read_idx + available) % AUDIO_BUFFER_SIZE;
    }
    // 攒满一帧发送
    else if(available >= AUDIO_FRAME_SAMPLES)
    {
        audio_frame_send(&audio_sample_buffer[audio_read_idx], AUDIO_FRAME_SAMPLES);
        audio_read_idx = (audio_read_idx + AUDIO_FRAME_SAMPLES) % AUDIO_BUFFER_SIZE;
    }

    // 解析识别结果
    parse_esp32_result();
}