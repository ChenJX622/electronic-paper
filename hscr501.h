#ifndef __HCSR501_H
#define __HCSR501_H
#include "hal_data.h"
#include <stdint.h>
fsp_err_t hcsr501_init(void);
fsp_err_t hcsr501_read(uint8_t* status);

#endif