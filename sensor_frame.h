#ifndef __SENSOR_FRAME_H__
#define __SENSOR_FRAME_H__

#include "hal_data.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/************************* 宏定义 *************************/
#define MPU6050_ADDR        0x68
#define MPU6050_PWR_MGMT_1  0x6B
#define MPU6050_SMPLRT_DIV  0x19
#define MPU6050_GYRO_CONFIG 0x1B
#define MPU6050_ACC_CONFIG  0x1C
#define MPU6050_DATA_START  0x3B
#define MPU6050_WHO_AM_I    0x75

#define HCSR501_PIN         BSP_IO_PORT_00_PIN_04
#define SHAKE_THRESHOLD     8000
#define SHAKE_COOLDOWN_MS   1000

/************************* 数据结构体 *************************/
typedef struct {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    float   temp_c;
} mpu6050_data_t;

/************************* 对外接口函数声明 *************************/
fsp_err_t system_peripheral_init(void);
fsp_err_t mpu6050_init(void);
fsp_err_t mpu6050_read_data(mpu6050_data_t *data);
uint8_t mpu6050_check_shake(mpu6050_data_t *new_data);
uint8_t hcsr501_get_status(void);
uint32_t get_sys_tick_ms(void);
#endif