/**
 *@file buzzer.h
 *@author Ribin Huang (you@domain.com)
 *@brief 
 *@version 1.0
 *@date 2021-05-20
 *
 *Copyright (c) 2019 - 2020 Fractal Auto Technology Co.,Ltd.
 *All right reserved.
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License version 2 as
 *published by the Free Software Foundation.
 *
 */
#if defined(BOARD_XH_5102)	
#ifndef __BUZZER_H__
#define __BUZZER_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
    BUZ_ACT_NONE = 0,
    BUZ_ACT_SHORT_RING,
    BUZ_ACT_LONG_RING,
	BUZ_ACT_SERIAL_RING
}eBuzAction_t;

void Buzzer_Init(void);
void Buzzer_Ctrl(eBuzAction_t action, uint16_t actionTimes);

#ifdef __cplusplus
}
#endif

#endif
#endif
