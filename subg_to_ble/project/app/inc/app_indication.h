/**
 *@file app_indication.h
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
#ifndef __APP_INDICATION_H__
#define __APP_INDICATION_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
	INDICATE_IDLE = 0,
	INDICATE_ADV,
	INDICATE_CONNECTED,
	INDICATE_DISCONNECTED,
	INDICATE_LOW_POWER,
	INDICATE_NONE_SN,
	INDICATE_FAC_TEST_START,
	INDICATE_FAC_TEST_STOP
}eIndicationType_t;

void Idc_Init(void);
void Idc_SetType(eIndicationType_t type);

#ifdef __cplusplus
}
#endif

#endif
