/**
 *@file kit_heap.c
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
#include <stdint.h>
#include <stbool.h>

#include "kit_config.h"

__attribute__((aligned(4))) static uint8_t heap[HEAP_SIZE];
static uint16_t heapAllocTable[HEAP_ALLOC_TABLE_SIZE];
static volatile bool heapReady = false;

/*memory usage(0~100)*/
uint32_t Kit_MemUsage(void)  
{  
    uint32_t usage = 0;  
    uint32_t i;
	
    if(!heapReady)
    {
        return 0;
    }
	
    for(i = 0; i < HEAP_ALLOC_TABLE_SIZE; i++)  
	{  
		if(heapAllocTable[i])
		{
			usage++; 
		}
    } 
	
    return (usage * 100) / HEAP_ALLOC_TABLE_SIZE;  
}  

void *Kit_Malloc(uint32_t size)
{  
	int offset = 0;  
    uint32_t blockNum;
	uint32_t blockCnt = 0;
    uint32_t i;  
	
    if(size == 0)
    {
        return NULL;
    }
	
    if(!heapReady)
    {
        memset(heap, 0, HEAP_SIZE);
        memset(heapAllocTable, 0, HEAP_ALLOC_TABLE_SIZE);
        heapReady = true;
    }
	
    blockNum = ((size - 1) / HEAP_BLOCK_SIZE) + 1;
    	
    for(offset = HEAP_ALLOC_TABLE_SIZE - 1; offset >= 0; offset--)  
    {     
		if(!heapAllocTable[offset])
		{
			 blockCnt++;
		}
		else 
		{
			blockCnt = 0;
		}
		
		if(blockCnt == blockNum)
		{
            for(i = 0; i < blockNum; i++)
            {  
                heapAllocTable[offset + i] = blockNum;  
            }  
            return (void*)((uint32_t)heap + (offset * HEAP_BLOCK_SIZE));  
		}
    }  
	
    return NULL;  
}  

void Kit_Free(void *pAddr)  
{  
    int offset;
    uint32_t blockId;
    uint32_t blockNum;
    uint32_t i;  
	
	if(pAddr == NULL)
	{
		return;  
	}
	
    if (!heapReady)
    {
        return;
    }
	
	offset = (uint32_t)pAddr - (uint32_t)heap;     
	
    if(offset < HEAP_SIZE) 
    {  
        blockId = offset / HEAP_BLOCK_SIZE;  
        blockNum = heapAllocTable[blockId];	
        
        for(i = 0; i < blockNum; i++) 
        {  
            heapAllocTable[blockId + i] = 0;  
        }  
    }
	else 
    {
    	return;  
	}
}  

void *Kit_Realloc(void *pOldAddr, uint32_t size)  
{  
    void *pNewAddr;
	
    pNewAddr = Kit_Malloc(size);
	
    if(pNewAddr)
    {  									   
		memcpy(pNewAddr, pOldAddr, size);   
		Kit_Free(pOldAddr);
		
		return pNewAddr;
    }  
	
	return NULL;
}



