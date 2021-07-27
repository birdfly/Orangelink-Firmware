/**
 *@file app_subg.h
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
#ifndef __APP_SUBG_H__
#define __APP_SUBG_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	SUBG_MODE_OMNIPOD = 0,
	SUBG_MODE_MINIMED_NAS,
	SUBG_MODE_MINIMED_WWL
}eSubgMode_t;

typedef enum
{
	SUBG_RX_OK = 0,
	SUBG_RX_TIMEOUT,
	SUBG_RX_INT
}eSubgRxStatus_t;

void Subg_SetMode(eSubgMode_t mode);
eSubgMode_t Subg_GetMode(void);
void Subg_SendPkt(uint8_t *pBuf, uint16_t len, uint8_t repeatCnt, uint16_t repeatIntvl, uint16_t preambleExt); 
eSubgRxStatus_t Subg_GetPkt(uint8_t *pRxBuf, uint8_t *pRxLen, uint32_t timeout, uint8_t usePktLen); 
void Subg_SetFreq(uint32_t freqHz);
void Subg_CfgRf(void);
void Subg_Init(void);
int Subg_GetRssi(void); 
uint16_t Subg_GetRxPktCnt(void); 
uint16_t Subg_GetTxPktCnt(void); 
void Subg_SetPreamble(uint16_t preamble); 
void Subg_SetPktLen(uint8_t len); 
void Subg_SetIntFlg(void);
void Subg_ClrIntFlg(void);
//void Subg_Test(void);

#ifdef __cplusplus
}
#endif

#endif


