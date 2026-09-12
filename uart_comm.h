#ifndef __UART_COMM_H
#define __UART_COMM_H
#include "hal_data.h"
#include <stdint.h>

// 瑞萨↔ESP32通信协议
#define FRAME_HEAD_1  0xAA
#define FRAME_HEAD_2  0x55
#define FRAME_TYPE_AUDIO 0x01
#define FRAME_TAIL    0xEE
#define FRAME_TYPE_TEXT  0x02
extern volatile bool g_text_received;
extern uint8_t g_text_buf[256];
extern uint16_t g_text_len;
fsp_err_t uart_comm_init(void);

fsp_err_t uart_comm_receive_result(char *result_buf, uint32_t buf_len);
void uart_comm_rx_start(void);
#endif