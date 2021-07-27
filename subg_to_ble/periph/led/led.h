/**
 *@file led.h
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
#ifndef __LED_H__
#define __LED_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	LED_ACT_NONE,
	LED_ACT_RED_LIGHT,
	LED_ACT_YELLOW_LIGHT,
	LED_ACT_RED_TWINKLE,
	LED_ACT_YELLOW_TWINKLE,
	LED_ACT_CROSS_TWINKLE
}eLedAction_t;

void Led_Init(void);
void Led_Ctrl(eLedAction_t action, uint16_t actionTimes);

#ifdef __cplusplus
}
#endif

#endif

