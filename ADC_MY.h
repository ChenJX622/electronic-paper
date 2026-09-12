#ifndef __ADC_MY_H
#define __ADC_MY_H
#include "hal_data.h"
#include <stdint.h>

void ADC_Init(void);
double Read_ADC_Voltage_Value(void);
uint16_t ADC_Read_Raw(void);
#endif
