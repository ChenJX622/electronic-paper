#include "hal_data.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "display.h"
#include "mpu6050.h"
#include "hscr501.h"
#include "asr_pro.h"

// ==================== 回调函数 ====================
void adc0_callback(adc_callback_args_t * p_args) { (void)p_args; }
void gpt0_callback(timer_callback_args_t * p_args) { (void)p_args; }

// ==================== UART7 调试串口 ====================
static volatile bool uart7_tx_done = false;
void uart7_callback(uart_callback_args_t * p_args)
{
    if (UART_EVENT_TX_COMPLETE == p_args->event) uart7_tx_done = true;
}
static fsp_err_t uart7_wait_tx(uint32_t timeout_ms)
{
    while((!uart7_tx_done) && (timeout_ms-- > 0)) R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    if(uart7_tx_done == false) return FSP_ERR_TIMEOUT;
    uart7_tx_done = false;
    return FSP_SUCCESS;
}
int fputc(int ch, FILE *f)
{
    (void)f;
    g_uart7.p_api->write(g_uart7.p_ctrl, (const uint8_t *)&ch, 1);
    uart7_wait_tx(10);
    return ch;
}

// ==================== MPU6050 摇晃检测 ====================
#define SHAKE_THRESHOLD 4000
#define SHAKE_COOLDOWN_MS  800
static int16_t last_accel_x = 0, last_accel_y = 0, last_accel_z = 0;
static bool mpu_first_read = true;
static uint32_t shake_cooldown_counter = 0;
bool mpu6050_check_shake(mpu6050_data_t *current_data)
{
    if(current_data == NULL) return false;
    if(shake_cooldown_counter > 0) shake_cooldown_counter--;
    if(mpu_first_read)
    {
        last_accel_x = current_data->acc_x;
        last_accel_y = current_data->acc_y;
        last_accel_z = current_data->acc_z;
        mpu_first_read = false;
        return false;
    }
    int32_t delta_x = abs(current_data->acc_x - last_accel_x);
    int32_t delta_y = abs(current_data->acc_y - last_accel_y);
    int32_t delta_z = abs(current_data->acc_z - last_accel_z);
    last_accel_x = current_data->acc_x;
    last_accel_y = current_data->acc_y;
    last_accel_z = current_data->acc_z;
    
    static uint32_t mpu_debug_cnt = 0;
    mpu_debug_cnt++;
    if(mpu_debug_cnt % 100 == 0)
    {
        printf("【MPU】X:%5d Y:%5d Z:%5d | 阈值:%d\r\n", delta_x, delta_y, delta_z, SHAKE_THRESHOLD);
    }

    if((delta_x > SHAKE_THRESHOLD || delta_y > SHAKE_THRESHOLD || delta_z > SHAKE_THRESHOLD) 
        && (shake_cooldown_counter == 0))
    {
        shake_cooldown_counter = SHAKE_COOLDOWN_MS / 10;
        return true;
    }
    return false;
}

// ==================== 核心配置宏 ====================
#define HUMAN_HOLD_TIME_MS 20000
#define IMAGE_LOOP_DELAY_MS 2000
#define HUMAN_DEBOUNCE_CNT 5
#define NO_HUMAN_DEBOUNCE_CNT 10
#define TOTAL_IMAGE_NUM 6

// ==================== 核心状态变量 ====================
static uint8_t current_image = 0;
static uint32_t human_hold_counter = 0;
static uint32_t auto_loop_counter = 0;
static uint8_t human_debounce = 0;
static uint8_t no_human_debounce = 0;
static bool human_present_stable = false;
static bool in_human_mode = false; // 有人模式标志（核心锁）

// ==================== 辅助函数：切下一张图 ====================
void switch_to_next_image(void)
{
    current_image++;
    if(current_image >= TOTAL_IMAGE_NUM) current_image = 0;
    printf("【切图】切换到图片 %d\r\n", current_image);
    display_show_image(current_image);
}

// ==================== 主函数【修复所有bug】 ====================
void hal_entry(void)
{
    fsp_err_t err;
    uint8_t hcsr_raw = 0;
    uint8_t asr_cmd = 0;
    bool shake_detected = false;
    mpu6050_data_t mpu_data;

    R_BSP_SoftwareDelay(2000, BSP_DELAY_UNITS_MILLISECONDS);
    printf("\r\n=========================================\r\n");
    printf("  智能相框 - bug修复版\r\n");
    printf("  1. 【最高】有人模式：摇晃/语音切图，持续有人永久刷新20秒\r\n");
    printf("  2. 【修复】红外防抖保护，保持期内不误判无人\r\n");
    printf("  3. 【修复】倒计时到0才进入轮播，不会提前退出\r\n");
    printf("=========================================\r\n");

    // 初始化外设
    err = g_uart7.p_api->open(g_uart7.p_ctrl, g_uart7.p_cfg);
    if(FSP_SUCCESS != err) while(1);
    err = g_i2c2.p_api->open(g_i2c2.p_ctrl, g_i2c2.p_cfg);
    if(FSP_SUCCESS != err) while(1);
    hcsr501_init();
    mpu6050_init();
    display_init();
    err = asr_pro_init();
    if(FSP_SUCCESS != err) while(1);
    
    current_image = 0;
    display_show_image(current_image);

    while(1)
    {
        // ==============================================
        // 【第一步：红外防抖处理【核心修复】
        // ==============================================
        hcsr501_read(&hcsr_raw);
        // 【新增】打印红外原始值，方便调试
        static uint32_t ir_debug_cnt = 0;
        ir_debug_cnt++;
        if(ir_debug_cnt % 50 == 0)
        {
            printf("【红外】原始值:%d | 有人防抖:%d | 无人防抖:%d | 稳定有人:%s\r\n",
                   hcsr_raw, human_debounce, no_human_debounce,
                   human_present_stable ? "是" : "否");
        }

        // 【修复1】有人模式下，红外防抖保护：只有连续读到1才判定有人，连续读到0才判定无人
        if(hcsr_raw == 1)
        {
            // 读到有人：累加有人防抖，清零无人防抖
            human_debounce++;
            no_human_debounce = 0;
            if(human_debounce >= HUMAN_DEBOUNCE_CNT)
            {
                human_debounce = HUMAN_DEBOUNCE_CNT;
                human_present_stable = true;
            }
        }
        else
        {
            // 【修复2】只有不在有人模式下，才累加无人防抖！！！
            // 有人模式下，哪怕红外偶尔读0，也不判定无人
            if(!in_human_mode)
            {
                no_human_debounce++;
                human_debounce = 0;
                if(no_human_debounce >= NO_HUMAN_DEBOUNCE_CNT)
                {
                    no_human_debounce = NO_HUMAN_DEBOUNCE_CNT;
                    human_present_stable = false;
                }
            }
            // 有人模式下，红外读0，只清零有人防抖，不修改稳定有人标志
            else
            {
                human_debounce = 0;
            }
        }

        // ==============================================
        // 第二步：读取传感器状态
        // ==============================================
        shake_detected = false;
        if(mpu6050_read(&mpu_data) == FSP_SUCCESS)
        {
            shake_detected = mpu6050_check_shake(&mpu_data);
        }
        bool asr_triggered = asr_pro_read_cmd(&asr_cmd);

        // ==============================================
        // 【第三步：核心状态机【优先级从高到低，完全修复】
        // ==============================================

        // --------------------------
        // 【最高优先级】有人模式（只要in_human_mode为true，就锁死在这里）
        // --------------------------
        if(in_human_mode)
        {
            // 【核心要求】只要检测到稳定有人，20秒计数器永久刷新！！！
            if(human_present_stable)
            {
                human_hold_counter = HUMAN_HOLD_TIME_MS / 10;
            }
            // 没有稳定有人，才开始倒计时
            else
            {
                human_hold_counter--;
            }

            // 【最高级响应】摇晃/语音切图（同级）
            if(shake_detected || (asr_triggered && asr_cmd == ASR_CMD_NEXT_IMAGE))
            {
                if(shake_detected) printf("【触发】摇晃切图\r\n");
                if(asr_triggered && asr_cmd == ASR_CMD_NEXT_IMAGE) printf("【触发】语音切图\r\n");
                switch_to_next_image();
            }

            // 调试打印
            static uint32_t human_debug_cnt = 0;
            human_debug_cnt++;
            if(human_debug_cnt % 100 == 0)
            {
                uint32_t remain_s = human_hold_counter / 100;
                printf("【有人模式】当前图片:%d | 稳定有人:%s | 剩余保持时间:%d秒\r\n",
                       current_image,
                       human_present_stable ? "是" : "否",
                       remain_s);
            }

            // 【修复3】只有倒计时到0，才退出有人模式！！！
            if(human_hold_counter == 0)
            {
                printf("【退出有人模式】20秒倒计时结束，进入无人轮播\r\n");
                in_human_mode = false;
                auto_loop_counter = 0;
                human_present_stable = false;
                no_human_debounce = 0;
            }

            // 本轮循环结束，跳过后面的无人逻辑
            R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
            continue;
        }

        // --------------------------
        // 【次高优先级】刚检测到稳定有人，进入有人模式
        // --------------------------
        if(human_present_stable && !in_human_mode)
        {
            printf("【进入有人模式】检测到稳定有人，默认显示Page0，20秒保持期启动\r\n");
            current_image = 0;
            display_show_image(current_image);
            in_human_mode = true;
            human_hold_counter = HUMAN_HOLD_TIME_MS / 10;

            R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
            continue;
        }

        // --------------------------
        // 【最低优先级】无人模式，每2秒轮播
        // --------------------------
        else
        {
            auto_loop_counter++;
            if(auto_loop_counter >= (IMAGE_LOOP_DELAY_MS / 10))
            {
                switch_to_next_image();
                auto_loop_counter = 0;
            }
            
            static uint32_t idle_debug_cnt = 0;
            idle_debug_cnt++;
            if(idle_debug_cnt % 200 == 0)
            {
                printf("【无人模式】当前图片:%d | 轮播中...\r\n", current_image);
            }
        }

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    }

#if BSP_TZ_SECURE_BUILD
    R_BSP_NonSecureEnter();
#endif
}
