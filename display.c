#include "display.h"
#include <string.h>
#include <stdio.h>

// ==================== 全局变量 ====================
static volatile bool uart9_tx_done = false;
/* [修改2] 当前页面初始值：从1改为0 */
static uint8_t current_image = 0;
static display_state_t current_state = DISPLAY_STATE_LOOP;
static uint32_t display_loop_counter = 0;

// ==================== UART9 回调函数 ====================
void uart9_callback(uart_callback_args_t *p_args)
{
    if (UART_EVENT_TX_COMPLETE == p_args->event)
    {
        uart9_tx_done = true;
    }
}

// ==================== 初始化显示屏 ====================
void display_init(void)
{
    fsp_err_t err;
    
    err = R_SCI_UART_Open(&DISPLAY_UART, &DISPLAY_UART_CFG);
    assert(FSP_SUCCESS == err);
    
    /* [修改2] 初始显示页面：从1改为0 */
    display_show_image(0);
    printf("【显示屏】初始化完成，默认显示 Page 0\r\n");
}

// ==================== 强制清零循环计数器 ====================
void display_reset_loop_counter(void)
{
    display_loop_counter = 0;
    printf("【显示屏】循环计数器已清零\r\n");
}

// ==================== 【核心修正】淘晶驰页面切换指令 ====================
void display_show_image(uint8_t image_num)
{
    uint8_t tx_buf[20] = {0};
    uint16_t len;
    
    /* [修改3] 页面范围保护：从 <1||>6 改为 <0||>=6（适配0-5） */
    if (image_num >= 6)
    {
        image_num = 0;
    }
    
    // 构造淘晶驰标准指令："page X" + 0xFF 0xFF 0xFF
    len = sprintf((char*)tx_buf, "page %d", image_num);
    tx_buf[len++] = 0xFF;
    tx_buf[len++] = 0xFF;
    tx_buf[len++] = 0xFF;
    
    uart9_tx_done = false;
    fsp_err_t err = R_SCI_UART_Write(&DISPLAY_UART, tx_buf, len);
    if(err != FSP_SUCCESS)
    {
        printf("【显示屏】发送失败，错误码：%d\r\n", err);
        return;
    }
    
    volatile uint32_t timeout = 100000;
    while((!uart9_tx_done) && (timeout-- > 0));
    
    if(uart9_tx_done == false)
    {
        printf("【显示屏】发送超时！请检查RASC里UART9的回调是否为uart9_callback\r\n");
    }
    else
    {
        printf("【显示屏】已切换到页面：%d\r\n", image_num);
    }
    
    current_image = image_num;
}

// ==================== 切换到下一个页面 ====================
void display_next_image(void)
{
    uint8_t next_img = current_image + 1;
    /* [修改3] 图片范围判断：从 >TOTAL_IMAGES 改为 >=6（适配0-5） */
    if (next_img >= 6)
    {
        next_img = 0;
    }
    display_show_image(next_img);
}

// ==================== 主循环状态机处理 ====================
void display_process(bool human_detected, bool shake_detected)
{
    if(human_detected)
    {
        display_loop_counter = 0;
    }

    if (shake_detected && (current_state != DISPLAY_STATE_SHAKE))
    {
        current_state = DISPLAY_STATE_SHAKE;
        display_next_image();
        R_BSP_SoftwareDelay(300, BSP_DELAY_UNITS_MILLISECONDS);
        return;
    }
    
    if (human_detected)
    {
        if (current_state != DISPLAY_STATE_HUMAN)
        {
            current_state = DISPLAY_STATE_HUMAN;
            /* [修改4] 人体检测切图：从1改为0 */
            display_show_image(0);
        }
        return;
    }
    
    current_state = DISPLAY_STATE_LOOP;
    display_loop_counter++;
    
    if (display_loop_counter >= (IMAGE_LOOP_DELAY_MS / 10))
    {
        display_next_image();
        display_loop_counter = 0;
    }
}
