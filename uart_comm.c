#include "uart_comm.h"
#include <string.h>

#define COMM_UART g_uart8

// 【修改 1】删除原来复杂的帧定义，只保留 ESP32 期望的极简帧头
#define FRAME_HEAD_1 0xAA  // 必须和 ESP32 一致
#define FRAME_HEAD_2 0x55  // 必须和 ESP32 一致

static volatile bool uart_tx_done = false;

fsp_err_t uart_comm_init(void)
{
    return COMM_UART.p_api->open(COMM_UART.p_ctrl, COMM_UART.p_cfg);
}

// 【修改 2】完全重写发送函数，协议 100% 匹配 ESP32
fsp_err_t uart_comm_send_audio(int16_t *audio_buf, uint32_t sample_cnt)
{
    static uint8_t frame[1024];
    uint32_t idx = 0;
    uint32_t byte_len = sample_cnt * 2; // PCM 总字节数 = 采样点数 × 2

    // --------------------------
    // 【修改 2-1】极简帧头：0xAA 0x55
    // --------------------------
    frame[idx++] = FRAME_HEAD_1;
    frame[idx++] = FRAME_HEAD_2;

    // --------------------------
    // 【修改 2-2】长度：小端序（低字节在前，高字节在后）
    // --------------------------
    frame[idx++] = (uint8_t)(byte_len & 0xFF);         // 低字节
    frame[idx++] = (uint8_t)((byte_len >> 8) & 0xFF);  // 高字节

    // --------------------------
    // 【修改 2-3】删除：帧类型、校验和、帧尾（ESP32 不需要）
    // --------------------------

    // --------------------------
    // 【修改 2-4】PCM 数据：小端序（低字节在前，高字节在后）
    // --------------------------
    for(uint32_t i = 0; i < sample_cnt; i++)
    {
        int16_t s = audio_buf[i];
        frame[idx++] = (uint8_t)(s & 0xFF);         // 低字节
        frame[idx++] = (uint8_t)((s >> 8) & 0xFF);  // 高字节
    }

    // --------------------------
    // 【修改 2-5】关键：等待发送完成，避免丢包
    // --------------------------
    uart_tx_done = false;
    fsp_err_t err = COMM_UART.p_api->write(COMM_UART.p_ctrl, frame, idx);
    while(uart_tx_done == false); // 必须等 UART_EVENT_TX_COMPLETE

    return err;
}

volatile bool g_text_received = false;
uint8_t g_text_buf[256] = {0};
uint16_t g_text_len = 0;

// 接收状态机（【修改 3】保留你原有的接收功能，完全不动）
typedef enum {
    RX_WAIT_HEAD1,
    RX_WAIT_HEAD2,
    RX_WAIT_TYPE,
    RX_WAIT_LEN_H,
    RX_WAIT_LEN_L,
    RX_WAIT_CHECKSUM,
    RX_WAIT_DATA,
    RX_WAIT_TAIL
} RxUartState;
static RxUartState rx_state = RX_WAIT_HEAD1;
static uint8_t  rx_frame_type = 0;
static uint16_t rx_frame_len = 0;
static uint8_t  rx_checksum = 0;
static uint8_t  rx_checksum_calc = 0;
static uint16_t rx_data_idx = 0;

// UART8 回调函数（【修改 4】只保留发送完成标志，接收功能完全不动）
void uart8_callback(uart_callback_args_t *p_args)
{
    if(UART_EVENT_TX_COMPLETE == p_args->event)
    {
        uart_tx_done = true; // 【修改 4-1】保留发送完成标志
    }
    else if(UART_EVENT_RX_CHAR == p_args->event)
    {
        // 【修改 4-2】你原有的接收状态机完全保留，不动
        uint8_t data = (uint8_t)p_args->data;
        switch(rx_state)
        {
            case RX_WAIT_HEAD1:
                if(data == FRAME_HEAD_1)
                {
                    rx_state = RX_WAIT_HEAD2;
                    rx_checksum_calc = data;
                }
                break;
            case RX_WAIT_HEAD2:
                if(data == FRAME_HEAD_2)
                {
                    rx_state = RX_WAIT_TYPE;
                    rx_checksum_calc ^= data;
                }
                else rx_state = RX_WAIT_HEAD1;
                break;
            case RX_WAIT_TYPE:
                rx_frame_type = data;
                rx_checksum_calc ^= data;
                rx_state = RX_WAIT_LEN_H;
                break;
            case RX_WAIT_LEN_H:
                rx_frame_len = (data << 8);
                rx_checksum_calc ^= data;
                rx_state = RX_WAIT_LEN_L;
                break;
            case RX_WAIT_LEN_L:
                rx_frame_len |= data;
                rx_checksum_calc ^= data;
                rx_state = RX_WAIT_CHECKSUM;
                break;
            case RX_WAIT_CHECKSUM:
                rx_checksum = data;
                if(rx_checksum == rx_checksum_calc)
                {
                    rx_data_idx = 0;
                    memset(g_text_buf, 0, sizeof(g_text_buf));
                    rx_state = RX_WAIT_DATA;
                }
                else rx_state = RX_WAIT_HEAD1;
                break;
            case RX_WAIT_DATA:
                if(rx_data_idx < 256)
                {
                    g_text_buf[rx_data_idx++] = data;
                }
                if(rx_data_idx >= rx_frame_len)
                {
                    rx_state = RX_WAIT_TAIL;
                }
                break;
            case RX_WAIT_TAIL:
                if(data == FRAME_TAIL && rx_frame_type == FRAME_TYPE_TEXT)
                {
                    g_text_len = rx_frame_len;
                    g_text_received = true;
                }
                rx_state = RX_WAIT_HEAD1;
                break;
            default:
                rx_state = RX_WAIT_HEAD1;
                break;
        }
    }
}

// 【修改 5】保留你原有的接收启动函数，完全不动
void uart_comm_rx_start(void)
{
    g_text_received = false;
    g_text_len = 0;
    rx_state = RX_WAIT_HEAD1;
}
