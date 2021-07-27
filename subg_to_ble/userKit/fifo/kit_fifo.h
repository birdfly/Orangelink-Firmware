/**
 *@file kit_fifo.h
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
#ifndef __KIT_FIFO_H__
#define __KIT_FIFO_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile struct 
{
	volatile uint32_t	bufSize;
	volatile uint8_t	*pFront;
	volatile uint8_t	*pTail;
	volatile uint8_t	*pBuf;
}stKitFifo_t;

typedef volatile struct 
{
	volatile uint32_t	elemSize;
	volatile uint32_t	sumCnt;
	volatile uint32_t	front;
	volatile uint32_t	tail;
	volatile void		*pBuf;
}stKitFifoStruct_t;

uint32_t Kit_FifoCreate(stKitFifo_t *pFifo, uint8_t *pBuf, uint32_t bufSize);
uint32_t Kit_FifoIn(stKitFifo_t *pFifo, uint8_t *pData, uint32_t len);
uint32_t Kit_FifoOut(stKitFifo_t *pFifo, uint8_t *pData, uint32_t len);
uint32_t Kit_FifoLenGet(stKitFifo_t *pFifo);

uint32_t Kit_FifoStructCreate(stKitFifoStruct_t *pFifoS, void *pBuf, uint32_t bufSize, uint16_t blockSize);
uint32_t Kit_FifoStructIn(stKitFifoStruct_t *pFifoS, void *pData, uint32_t blockCnt);
uint32_t Kit_FifoStructOut(stKitFifoStruct_t *pFifoS, void *pData, uint32_t blockCnt);
uint32_t Kit_FifoStructCntGet(stKitFifoStruct_t *pFifoS);

#ifdef __cplusplus
}
#endif

#endif

