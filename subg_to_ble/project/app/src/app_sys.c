/**
 *@file app_sys.c
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
#include "ocp.h"
#include "kit_log.h"

#define TAG "SYS"

void Sys_Idle(void)
{
	Pwr_MgmtIdle();
}

void Sys_Init(void)
{
	Log_Init();
	Rtc_Init();
	Pwr_MgmtInit();	
	Pwr_ResetReason();
	Dcdc_Enable();
	Wdt_Init();
	Timer_Init();
	Flash_Init();

	KIT_LOG(TAG, "Init OK!");
}

