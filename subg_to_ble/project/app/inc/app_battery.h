/**
 *@file app_battery.h
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
#ifndef __APP_BATTERY_H__
#define __APP_BATTERY_H__
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void Batt_Init(void);
uint8_t Batt_GetLevel(void);
bool Batt_IsLow(void);
uint16_t Batt_GetVoltage(void);

#ifdef __cplusplus
}
#endif

#endif
