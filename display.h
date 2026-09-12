#ifndef __DISPLAY_H
#define __DISPLAY_H

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

// ==================== 配置宏（适配淘晶驰） ====================
#define DISPLAY_UART        g_uart9_ctrl
#define DISPLAY_UART_CFG    g_uart9_cfg

/* [修改1] 总页面数：从5改为6（对应0-5共6张），注释更新 */
#define TOTAL_IMAGES        6
#define IMAGE_LOOP_DELAY_MS 2000

// ==================== 状态枚举 ====================
typedef enum
{
    DISPLAY_STATE_LOOP,
    DISPLAY_STATE_HUMAN,
    DISPLAY_STATE_SHAKE
} display_state_t;

// ==================== 函数声明 ====================
void display_init(void);
/* [修改1] 注释更新：支持0-5 */
void display_show_image(uint8_t image_num);
void display_process(bool human_detected, bool shake_detected);
void display_next_image(void);
void display_reset_loop_counter(void);

#endif /* __DISPLAY_H */