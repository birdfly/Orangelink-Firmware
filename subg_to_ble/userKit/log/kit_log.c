/**
 *@file kit_log.c
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
#include "kit_log.h"

#ifdef KIT_LOG_SUPORT
void Kit_PrintBytes(const char *pTag, const char *pMsg, const uint8_t *pBuf, uint16_t len) 
{
	KIT_LOG_RAW("[%s] %s", pTag, pMsg)
		
	for (uint16_t i = 0; i < len; i++) 
	{
		KIT_LOG_RAW(" %02X", pBuf[i]);
	}
	
	KIT_LOG_RAW(", len: %d.", len);
	KIT_LOG_RAW("\r\n");
}
#else
void Kit_PrintBytes(const char *pTag, const char *pMsg, const uint8_t *pBuf, uint16_t len) 
{
}
#endif


