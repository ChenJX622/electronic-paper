#include "hscr501.h"

// 定义HC-SR501连接的IO引脚
#define HCSR501_PIN         BSP_IO_PORT_00_PIN_04

fsp_err_t hcsr501_init(void)
{
    fsp_err_t err;
    
    // 配置IO端口为输入模式
    err = R_IOPORT_PinCfg(&g_ioport_ctrl, HCSR501_PIN, IOPORT_CFG_PORT_DIRECTION_INPUT);
    if (err != FSP_SUCCESS) return err;
    
    // 仅保留极短延时用于硬件稳定（10ms）
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    
    return FSP_SUCCESS;
}

fsp_err_t hcsr501_read(uint8_t* status)
{
    if (status == NULL) return FSP_ERR_INVALID_ARGUMENT;
    
    bsp_io_level_t level;
    fsp_err_t err = R_IOPORT_PinRead(&g_ioport_ctrl, HCSR501_PIN, &level);
    if (err != FSP_SUCCESS) return err;
    
    // HC-SR501：检测到人体输出高电平，未检测到输出低电平
    *status = (level == BSP_IO_LEVEL_HIGH) ? 1 : 0;
    
    return FSP_SUCCESS;
}