/**
 *@file kit_delay.c
 *@author Ribin Huang (you@domain.com)
 *@brief 
 *@version 1.0
 *@date 2021-01-05
 *
 *Copyright (c) 2019 - 2020 Fractal Auto Technology Co.,Ltd.
 *All right reserved.
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License version 2 as
 *published by the Free Software Foundation.
 *
 */
#include <stdint.h>

#include "nrf_delay.h"

void Kit_DelayMs(uint32_t ms)
{
	nrf_delay_ms(ms);
}

void Kit_DelayUs(uint32_t us)
{
	nrf_delay_us(us);
}

