#ifndef __ASR_PRO_H
#define __ASR_PRO_H

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

// ==================== 配置宏 ====================
#define ASR_PRO_UART        g_uart5_ctrl  // UART5 控制块
#define ASR_PRO_UART_CFG    g_uart5_cfg   // UART5 配置
#define ASR_BAUD_RATE       9600          // ASR PRO 默认波特率

// 语音指令ID定义
#define ASR_CMD_NEXT_IMAGE  12             // "下一张图片" 指令ID

// ==================== 函数声明 ====================
// 初始化 ASR PRO 模块（UART5）
fsp_err_t asr_pro_init(void);

// 非阻塞读取语音指令（返回 true 表示读到有效指令）
bool asr_pro_read_cmd(uint8_t *cmd_id);

#endif /* __ASR_PRO_H */