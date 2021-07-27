/**
 *@file ocp.h
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
#ifndef __OCP_H__
#define __OCP_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Flash_Read(void* pDest, const uint32_t srcAddr, uint32_t len);
void Flash_Write(uint32_t pageAddr, void const * pData, uint32_t len);
void Flash_Init(void);
void wdt_feed(void *pContext);
void Wdt_Init(void);
void Pwr_MgmtInit(void);
void Pwr_MgmtIdle(void);
void Pwr_ResetReason(void);
void Log_Init(void);
void Timer_Init(void);
uint32_t Timer_GetCnt(void);
void Rtc_Init(void);
void Dcdc_Enable(void);

#ifdef __cplusplus
}
#endif

#endif
