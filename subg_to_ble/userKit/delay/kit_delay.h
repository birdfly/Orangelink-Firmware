/**
 *@file kit_delay.h
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
#ifndef __KIT_DELAY_H__
#define __KIT_DELAY_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Kit_DelayMs(uint32_t ms);
void Kit_DelayUs(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif


