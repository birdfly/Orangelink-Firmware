/**
 *@file kit_fifo.c
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
#include <string.h>

#include "kit_fifo.h"
#include "kit_assert.h"

uint32_t Kit_FifoCreate(stKitFifo_t *pFifo, uint8_t *pBuf, uint32_t bufSize)
{
    KIT_ASSERT(pFifo);
    KIT_ASSERT(pBuf);
    KIT_ASSERT(bufSize);

    pFifo->bufSize = bufSize;
    pFifo->pBuf = pBuf;
    pFifo->pFront = pBuf;
    pFifo->pTail = pBuf;

    return 1;
}

uint32_t Kit_FifoIn(stKitFifo_t *pFifo, uint8_t *pData, uint32_t len)
{
    volatile uint8_t *pTail = NULL;
    
    uint32_t i = 0;
    
    KIT_ASSERT(pData);
    KIT_ASSERT(pFifo);
    KIT_ASSERT(pFifo->pBuf);
    KIT_ASSERT(pFifo->pFront);
    KIT_ASSERT(pFifo->pTail);

    pTail = pFifo->pTail;
    
    for(i = 0; i < len; i++)
    {
        if(++pTail >= pFifo->pBuf + pFifo->bufSize)
        {
            pTail = pFifo->pBuf;
        }
        if(pTail == pFifo->pFront) 
        {
            break;
        }
        
        *pFifo->pTail = *pData++;
        pFifo->pTail = pTail;
    }
    
    return i;
}

uint32_t Kit_FifoOut(stKitFifo_t *pFifo, uint8_t *pData, uint32_t len)
{
    uint32_t i = 0;
    
    KIT_ASSERT(pData);
    KIT_ASSERT(pFifo);
    KIT_ASSERT(pFifo->pBuf);
    KIT_ASSERT(pFifo->pFront);
    KIT_ASSERT(pFifo->pTail);

    while((pFifo->pFront != pFifo->pTail) && (i < len) && (i < pFifo->bufSize))
    {
        pData[i++] = *pFifo->pFront++;
        if(pFifo->pFront >= pFifo->pBuf + pFifo->bufSize) 
        {
            pFifo->pFront = pFifo->pBuf;
        }
    }

    return i;
}

uint32_t Kit_FifoLenGet(stKitFifo_t *pFifo)
{
    volatile uint8_t *pFront = NULL;
    uint32_t i = 0;
    
    KIT_ASSERT(pFifo);
    KIT_ASSERT(pFifo->pFront);
    KIT_ASSERT(pFifo->pTail);

    pFront = pFifo->pFront;

    while((pFront != pFifo->pTail) && (i < pFifo->bufSize))
    {
        i++;
        if(++pFront >= pFifo->pBuf + pFifo->bufSize) 
        {
            pFront = pFifo->pBuf;
        }
    }

    return i;
}

uint32_t Kit_FifoStructCreate(stKitFifoStruct_t *pFifoS, void *pBuf, uint32_t bufSize, uint16_t blockSize)
{
    KIT_ASSERT(pFifoS);
    KIT_ASSERT(pBuf);
    KIT_ASSERT(bufSize);
    KIT_ASSERT(blockSize);

    pFifoS->elemSize = blockSize;
    pFifoS->sumCnt = bufSize / blockSize;
    pFifoS->pBuf = pBuf;
    pFifoS->front = 0;
    pFifoS->tail = 0;
	
    return 1;
}

uint32_t Kit_FifoStructIn(stKitFifoStruct_t *pFifoS, void *pData, uint32_t blockCnt)
{
    uint32_t i = blockCnt;
    uint32_t mdwTail = 0;
    KIT_ASSERT(pFifoS);
    KIT_ASSERT(pFifoS->pBuf);
    KIT_ASSERT(pData);

    mdwTail = pFifoS->tail;
    for(i = 0; i < blockCnt; i++)
    {
        if(++mdwTail >= pFifoS->sumCnt)      
        {
            mdwTail = 0;
        }
        
        if(mdwTail == pFifoS->front)   
        {
            break; 
        }
        
        memcpy((uint8_t *)pFifoS->pBuf + pFifoS->tail * pFifoS->elemSize, pData, pFifoS->elemSize);

        pData = (uint8_t *)pData + pFifoS->elemSize;

        pFifoS->tail = mdwTail;
    }
    
    return i;
}

uint32_t Kit_FifoStructOut(stKitFifoStruct_t *pFifoS, void *pData, uint32_t blockCnt)
{
    uint32_t i = 0;
    
    KIT_ASSERT(pFifoS);
    KIT_ASSERT(pFifoS->pBuf);
    KIT_ASSERT(pData);

    while((pFifoS->front != pFifoS->tail) && (i < pFifoS->sumCnt) && (i < blockCnt))
    {
        memcpy(pData, (uint8_t *)pFifoS->pBuf + pFifoS->front * pFifoS->elemSize, pFifoS->elemSize);

        pData = (uint8_t *)pData + pFifoS->elemSize;
		
        i++;
		
        if(++pFifoS->front >= pFifoS->sumCnt) 
        {
            pFifoS->front = 0;
        }
    }

    return i;
}

uint32_t Kit_FifoStructCntGet(stKitFifoStruct_t *pFifoS)
{
    uint32_t i = 0;
    uint32_t mdwFront =0;
    
    KIT_ASSERT(pFifoS);
    KIT_ASSERT(pFifoS->pBuf);

    mdwFront = pFifoS->front;
    while((mdwFront != pFifoS->tail) && (i < pFifoS->sumCnt))
    {
        i++;
        if(++mdwFront >= pFifoS->sumCnt) 
        {
            mdwFront = 0;
        }
    }

    return i;
}

