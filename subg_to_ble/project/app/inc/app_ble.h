/**
 *@file app_ble.h
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
#ifndef __APP_BLE_H__
#define __APP_BLE_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
	BLE_STATE_ADV = 0,
	BLE_STATE_CONNECTED
}eBleState_t;

void Ble_Init(void);
void Ble_IpsNotifyRespCntAndSendData(uint8_t *data, int len);
eBleState_t Ble_GetState(void);
void Ble_NusSendData(uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif

