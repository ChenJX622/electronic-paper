#include "asr_pro.h"
#include <string.h>
#include <stdio.h>

// ==================== 全局变量 ====================
static volatile bool uart5_rx_done = false;  // UART5 接收完成标志
static volatile bool uart5_tx_done = false;  // UART5 发送完成标志
static uint8_t rx_buffer[1] = {0};            // 单字节接收缓冲区

void uart5_callback(uart_callback_args_t *p_args)
{
    if (UART_EVENT_RX_COMPLETE == p_args->event)
    {
        uart5_rx_done = true;
    }
    else if (UART_EVENT_TX_COMPLETE == p_args->event)
    {
        uart5_tx_done = true;
    }
}

// ==================== 初始化 ASR PRO 模块 ====================
fsp_err_t asr_pro_init(void)
{
    fsp_err_t err;
    
    // 打开 UART5
    err = R_SCI_UART_Open(&ASR_PRO_UART, &ASR_PRO_UART_CFG);
    if (FSP_SUCCESS != err)
    {
        printf("【ASR】UART5 打开失败，错误码：%d\r\n", err);
        return err;
    }
    
    // 启动第一次接收
    uart5_rx_done = false;
    err = R_SCI_UART_Read(&ASR_PRO_UART, rx_buffer, 1);
    if (FSP_SUCCESS != err)
    {
        printf("【ASR】启动接收失败，错误码：%d\r\n", err);
        return err;
    }
    
    printf("【ASR】初始化成功，波特率：%d\r\n", ASR_BAUD_RATE);
    return FSP_SUCCESS;
}

// ==================== 非阻塞读取语音指令 ====================
bool asr_pro_read_cmd(uint8_t *cmd_id)
{
    if (cmd_id == NULL) return false;

    // 检查是否收到数据
    if (uart5_rx_done)
    {
        *cmd_id = rx_buffer[0];
        
        // 启动下一次接收
        uart5_rx_done = false;
        R_SCI_UART_Read(&ASR_PRO_UART, rx_buffer, 1);
        
        // 打印调试信息
        printf("【ASR】收到语音指令 ID：%d\r\n", *cmd_id);
        return true;
    }
    
    return false;
}