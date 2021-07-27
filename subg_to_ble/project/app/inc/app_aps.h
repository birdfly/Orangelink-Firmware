/**
 *@file app_aps.h
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
#ifndef __APP_APS_H__
#define __APP_APS_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Aps_PutCmd(const uint8_t *pBuf, uint16_t len, int8_t rssi);
void Aps_Init(void);
void Aps_StartLoop(void);
void Aps_StopLoop(void);

#ifdef __cplusplus
}
#endif

#endif
