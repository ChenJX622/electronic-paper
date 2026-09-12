#include "sensor_frame.h"

/************************* 内部静态全局变量 *************************/
static volatile uint32_t g_sys_tick_ms = 0;
static volatile bool uart_tx_done = false;
static volatile bool i2c_tx_done = false;
static volatile bool i2c_rx_done = false;
static volatile bool i2c_error_flag = false;
static int16_t last_acc_x = 0, last_acc_y = 0, last_acc_z = 0;
static uint32_t last_shake_time = 0;


// 系统1ms计时中断
void SysTick_Handler(void)
{
    g_sys_tick_ms++;
}

// 调试串口UART7回调
void uart7_callback(uart_callback_args_t * p_args)
{
    if (UART_EVENT_TX_COMPLETE == p_args->event)
    {
        uart_tx_done = true;
    }
}

// I2C2通信回调
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

/************************* printf重定向 *************************/
int fputc(int ch, FILE *f)
{
    (void)f;
    g_uart7.p_api->write(g_uart7.p_ctrl, (const uint8_t *)&ch, 1);
    while((!uart_tx_done));
    uart_tx_done = false;
    return ch;
}

/************************* 内部静态工具函数 *************************/
uint32_t get_sys_tick_ms(void)
{
    // 直接返回你的系统计时全局变量（比如 g_sys_tick_ms）
    extern volatile uint32_t g_sys_tick_ms;  // 如果没在头文件声明，这里加 extern
    return g_sys_tick_ms;
}

static fsp_err_t i2c_wait_event(volatile bool *event_flag, uint32_t timeout_ms)
{
    while((!(*event_flag)) && (!i2c_error_flag) && (timeout_ms > 0))
    {
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        timeout_ms--;
    }
    if(i2c_error_flag) { i2c_error_flag = false; return FSP_ERR_ABORTED; }
    if(*event_flag == false) return FSP_ERR_TIMEOUT;
    *event_flag = false;
    return FSP_SUCCESS;
}

static fsp_err_t mpu6050_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t tx_buf[2] = {reg, data};
    fsp_err_t err = g_i2c2.p_api->write(g_i2c2.p_ctrl, tx_buf, 2, false);
    if(err != FSP_SUCCESS) return err;
    return i2c_wait_event(&i2c_tx_done, 10);
}

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

/************************* 对外接口函数实现 *************************/
fsp_err_t system_peripheral_init(void)
{
    fsp_err_t err;
    
    SysTick_Config(SystemCoreClock / 1000);
    
    err = g_uart7.p_api->open(g_uart7.p_ctrl, g_uart7.p_cfg);
    if(err != FSP_SUCCESS) return err;
    
    err = g_i2c2.p_api->open(g_i2c2.p_ctrl, g_i2c2.p_cfg);
    if(err != FSP_SUCCESS) return err;
    
    return FSP_SUCCESS;
}

fsp_err_t mpu6050_init(void)
{
    fsp_err_t err;
    uint8_t whoami = 0;
    
    err = mpu6050_read_regs(MPU6050_WHO_AM_I, &whoami, 1);
    if(err != FSP_SUCCESS || whoami != 0x68) return FSP_ERR_NOT_FOUND;
    
    // 1. 唤醒芯片
    err = mpu6050_write_reg(MPU6050_PWR_MGMT_1, 0x00);
    if(err != FSP_SUCCESS) return err;
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    
    // 2. ✅ 关键修复：关闭陀螺仪/加速度计待机
    err = mpu6050_write_reg(0x6C, 0x00); 
    if(err != FSP_SUCCESS) return err;
    
    // 3. 其他配置（保持不变）
    err = mpu6050_write_reg(MPU6050_SMPLRT_DIV, 0x07);
    if(err != FSP_SUCCESS) return err;
    err = mpu6050_write_reg(MPU6050_GYRO_CONFIG, 0x18); // ±2000°/s
    if(err != FSP_SUCCESS) return err;
    err = mpu6050_write_reg(MPU6050_ACC_CONFIG, 0x10);   // ±8g
    if(err != FSP_SUCCESS) return err;
    
    return FSP_SUCCESS;
}

fsp_err_t mpu6050_read_data(mpu6050_data_t *data)
{
    uint8_t rx_buf[14] = {0};
    fsp_err_t err = mpu6050_read_regs(MPU6050_DATA_START, rx_buf, 14);
    if(err != FSP_SUCCESS) return err;
    
    data->acc_x  = (int16_t)((rx_buf[0] << 8) | rx_buf[1]);
    data->acc_y  = (int16_t)((rx_buf[2] << 8) | rx_buf[3]);
    data->acc_z  = (int16_t)((rx_buf[4] << 8) | rx_buf[5]);
    int16_t temp_raw = (int16_t)((rx_buf[6] << 8) | rx_buf[7]);
    data->gyro_x = (int16_t)((rx_buf[8] << 8) | rx_buf[9]);
    data->gyro_y = (int16_t)((rx_buf[10] << 8) | rx_buf[11]);
    data->gyro_z = (int16_t)((rx_buf[12] << 8) | rx_buf[13]);
    
    data->temp_c = (float)temp_raw / 340.0f + 36.53f;
    return FSP_SUCCESS;
}

uint8_t mpu6050_check_shake(mpu6050_data_t *new_data)
{
    int32_t dx = abs(new_data->acc_x - last_acc_x);
    int32_t dy = abs(new_data->acc_y - last_acc_y);
    int32_t dz = abs(new_data->acc_z - last_acc_z);
    int32_t total_diff = dx + dy + dz;

    last_acc_x = new_data->acc_x;
    last_acc_y = new_data->acc_y;
    last_acc_z = new_data->acc_z;
    
    if (total_diff > SHAKE_THRESHOLD)
    {
        uint32_t now = get_sys_tick_ms();
        if (now - last_shake_time > SHAKE_COOLDOWN_MS)
        {
            last_shake_time = now;
            return 1;
        }
    }
    return 0;
}

uint8_t hcsr501_get_status(void)
{
    bsp_io_level_t level;
    R_IOPORT_PinRead(&g_ioport_ctrl, HCSR501_PIN, &level);
    return (level == BSP_IO_LEVEL_HIGH) ? 1 : 0;
}
