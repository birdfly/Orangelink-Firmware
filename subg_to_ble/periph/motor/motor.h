/**
 *@file motor.h
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
#ifndef __MOTOR_H__
#define __MOTOR_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
    MTR_ACT_NONE = 0,
    MTR_ACT_SHORT_VIBRATE,
    MTR_ACT_LONG_VIBRATE,
    MTR_ACT_SERIAL_VIBRATE1,
    MTR_ACT_SERIAL_VIBRATE2,
	MTR_ACT_SERIAL_VIBRATE3
}eMtrAction_t;

void Motor_Init(void);
void Motor_Ctrl(eMtrAction_t action, uint16_t actionTimes);

#ifdef __cplusplus
}
#endif

#endif
