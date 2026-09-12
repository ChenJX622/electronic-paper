#include "mpu6050.h"
#include <stdio.h>

/***********************************************************************************************************************
 * 全局变量与通信状态标志
 **********************************************************************************************************************/
static volatile bool i2c_tx_done = false;
static volatile bool i2c_rx_done = false;
static volatile bool i2c_error_flag = false;

/***********************************************************************************************************************
 * SCI2 I2C 中断回调函数（必须在 RASC 里配置 I2C2 的回调为这个函数）
 **********************************************************************************************************************/
void sci_i2c2_master_callback(i2c_master_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case I2C_MASTER_EVENT_TX_COMPLETE:
            i2c_tx_done = true;
            break;
        case I2C_MASTER_EVENT_RX_COMPLETE:
            i2c_rx_done = true;
            break;
        
        case I2C_MASTER_EVENT_ABORTED:
            i2c_error_flag = true;
            break;
        default:
            break;
    }
}

/***********************************************************************************************************************
 * 底层工具函数：等待 I2C 事件完成
 **********************************************************************************************************************/
static fsp_err_t i2c_wait_event(volatile bool *event_flag, uint32_t timeout_ms)
{
    while((!(*event_flag)) && (!i2c_error_flag) && (timeout_ms > 0))
    {
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        timeout_ms--;
    }
    
    if(*event_flag == false) return FSP_ERR_TIMEOUT;
    *event_flag = false;
    return FSP_SUCCESS;
}

/***********************************************************************************************************************
 * MPU6050 底层驱动：写寄存器
 **********************************************************************************************************************/
static fsp_err_t mpu6050_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t tx_buf[2] = {reg, data};
    fsp_err_t err = g_i2c2.p_api->write(g_i2c2.p_ctrl, tx_buf, 2, false);
    if(err != FSP_SUCCESS) return err;
    return i2c_wait_event(&i2c_tx_done, 10);
}

/***********************************************************************************************************************
 * MPU6050 底层驱动：连续读寄存器
 **********************************************************************************************************************/
static fsp_err_t mpu6050_read_regs(uint8_t reg, uint8_t *rx_buf, uint8_t len)
{
    fsp_err_t err = g_i2c2.p_api->write(g_i2c2.p_ctrl, &reg, 1, true);
    if(err != FSP_SUCCESS) return err;
    err = i2c_wait_event(&i2c_tx_done, 10);
    if(err != FSP_SUCCESS) return err;
    
    err = g_i2c2.p_api->read(g_i2c2.p_ctrl, rx_buf, len, false);
    if(err != FSP_SUCCESS) return err;
    return i2c_wait_event(&i2c_rx_done, 10);
}

/***********************************************************************************************************************
 * MPU6050 初始化
 **********************************************************************************************************************/
fsp_err_t mpu6050_init(void)
{
    fsp_err_t err;
    uint8_t whoami = 0;
    
    printf("【MPU6050】开始检测（当前I2C地址：0x68）...\r\n");
    
    // 读取 WHO_AM_I 寄存器验证芯片ID
    err = mpu6050_read_regs(MPU6050_WHO_AM_I, &whoami, 1);
    printf("【MPU6050】读到WHO_AM_I=0x%02X，错误码=%d\r\n", whoami, err);
    
    // 验证芯片ID（MPU6050 正确ID是0x68）
    if(err != FSP_SUCCESS || whoami != 0x68)
    {
        printf("【MPU6050】❌ 未在0x68地址找到，请检查接线或在RASC里将I2C2 slave地址改为0x69后重试\r\n");
        return FSP_ERR_NOT_FOUND;
    }
    
    // 唤醒 MPU6050，退出睡眠模式
    err = mpu6050_write_reg(MPU6050_PWR_MGMT_1, 0x00);
    if(err != FSP_SUCCESS) return err;
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    
    // 配置采样率 1kHz
    err = mpu6050_write_reg(MPU6050_SMPLRT_DIV, 0x07);
    if(err != FSP_SUCCESS) return err;
    
    // 配置陀螺仪量程 ±2000°/s
    err = mpu6050_write_reg(MPU6050_GYRO_CONFIG, 0x18);
    if(err != FSP_SUCCESS) return err;
    
    // 配置加速度计量程 ±8g
    err = mpu6050_write_reg(MPU6050_ACC_CONFIG, 0x10);
    if(err != FSP_SUCCESS) return err;
    
    printf("【MPU6050】✅ 初始化成功！\r\n");
    return FSP_SUCCESS;
}

/***********************************************************************************************************************
 * MPU6050 读取完整传感器数据
 **********************************************************************************************************************/
fsp_err_t mpu6050_read(mpu6050_data_t *data)
{
    if(data == NULL) return FSP_ERR_INVALID_ARGUMENT;
    
    uint8_t rx_buf[14] = {0};
    fsp_err_t err = mpu6050_read_regs(MPU6050_DATA_START, rx_buf, 14);
    if(err != FSP_SUCCESS) return err;
    
    // 拼接 16 位原始数据
    data->acc_x  = (int16_t)((rx_buf[0] << 8) | rx_buf[1]);
    data->acc_y  = (int16_t)((rx_buf[2] << 8) | rx_buf[3]);
    data->acc_z  = (int16_t)((rx_buf[4] << 8) | rx_buf[5]);
    int16_t temp_raw = (int16_t)((rx_buf[6] << 8) | rx_buf[7]);
    data->gyro_x = (int16_t)((rx_buf[8] << 8) | rx_buf[9]);
    data->gyro_y = (int16_t)((rx_buf[10] << 8) | rx_buf[11]);
    data->gyro_z = (int16_t)((rx_buf[12] << 8) | rx_buf[13]);
    
    // 温度转换（MPU6050 官方公式）
    data->temp_c = (float)temp_raw / 340.0f + 36.53f;
    
    return FSP_SUCCESS;
}