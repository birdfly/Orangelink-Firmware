/**
 *@file rf69.h
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
#ifndef __RF69_h__
#define __RF69_h__
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	RF69_MODE_NONE = 0,
	RF69_MODE_SLEEP,
	RF69_MODE_STANDBY,
	RF69_MODE_SYNTH,
	RF69_MODE_RX,
	RF69_MODE_TX
}eRf69Mode_t;

typedef enum
{
	RF69_DEV_FREQ433 = 0,
	RF69_DEV_FREQ916N868
}eRf69Dev_t;

typedef enum
{
	RF69_FREQ_433 = 0,
	RF69_FREQ_868,
	RF69_FREQ_916
}eRf69Freq_t;

void Rf69_SetMode(eRf69Dev_t dev, eRf69Mode_t newMode);
uint32_t Rf69_GetFreq(eRf69Dev_t dev);
void Rf69_SetFreq(eRf69Dev_t dev, uint32_t freqHz);
void Rf69_SetPowerLevel(eRf69Dev_t dev, uint8_t powerLevel);
int16_t Rf69_ReadRssi(eRf69Dev_t dev, bool forceTrigger);
bool Rf69_IsFifoEmpty(eRf69Dev_t dev);
bool Rf69_IsFifoFull(eRf69Dev_t dev);
bool Rf69_IsFifoOverThreshold(eRf69Dev_t dev);
void Rf69_ClearFifo(eRf69Dev_t dev);
void Rf69_XmitByte(eRf69Dev_t dev, uint8_t data);
void Rf69_XmitBuf(eRf69Dev_t dev, const uint8_t* pData, int len);
bool Rf69_PacketSeen(eRf69Dev_t dev);
uint8_t Rf69_RcvByte(eRf69Dev_t dev);
void Rf69_SetSeqOnOff(eRf69Dev_t dev, bool onOff);
void Rf69_SetPayloadLen(eRf69Dev_t dev, uint8_t len);
void Rf69_SetSyncOnOff(eRf69Dev_t dev, bool onOff);
void Rf69_SetPreambleSize(eRf69Dev_t dev, uint16_t size);
void Rf69_SetUnlimitedLenPkt(eRf69Dev_t dev);
void Rf69_SetOokBw250khz(eRf69Dev_t dev);
void Rf69_SetOokBw200khz(eRf69Dev_t dev);
void Rf69_SetDioMapping(eRf69Dev_t dev);
void Rf69_DevParaCfg(eRf69Dev_t dev, eRf69Freq_t freq);

#ifdef __cplusplus
}
#endif

#endif

