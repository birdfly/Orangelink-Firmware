/**
 *@file kit_queue.c
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

#include "kit_heap.h"

typedef struct
{
	struct stQueueItem_t *pNext;
	void *pData;
}stQueueItem_t;

typedef struct
{	
	stQueueItem_t *pHead;
	uint16_t itemSize;
	uint8_t cnt;
}stQueue_t;

void *Kit_QueueCreate(uint16_t itemSize)
{
	if(itemSize == 0)
	{
		return NULL;
	}

	stQueue_t *pQueueTmp = (stQueue_t *)Kit_Malloc(sizeof(stQueue_t));

	if(pQueueTmp == NULL)
	{
		return NULL;
	}

	pQueueTmp->pHead = NULL;
	pQueueTmp->itemSize = itemSize;
	pQueueTmp->cnt = 0;

	return pQueueTmp;
}

void Kit_QueueDestroy(void **ppQueue)
{
	stQueue_t *pQueueTmp = *((stQueue_t **)ppQueue);
	
	if(pQueueTmp == NULL)
	{
		return;
	}

	Kit_QueueClear(pQueueTmp);
	Kit_Free(pQueueTmp);

	*ppQueue = NULL;
}

int Kit_QueueAppend(void *pQueue, void *pItem)
{
	stQueue_t *pQueueTmp = pQueue;
	
	if (pQueueTmp == NULL)
	{
		return -1;
	}

	stQueueItem_t *pNewItem = (stQueueItem_t *)Kit_Malloc(sizeof(stQueueItem_t));
	if (pNewItem == NULL)
	{
		return -1;
	}
	
	pNewItem->pNext = NULL;

	pNewItem->pData = Kit_Malloc(pQueueTmp->itemSize);
	
	if (pNewItem->pData == NULL)
	{
		Kit_Free(pNewItem);
		return -1;
	}
	
	memcpy(pNewItem->pData, pItem, pQueueTmp->itemSize);

	stQueueItem_t **ppQueueTmp = &pQueueTmp->pHead;
	
	while (*ppQueueTmp != NULL)
	{
		ppQueueTmp = &((*ppQueueTmp)->pNext);
	} 		
	*ppQueueTmp = pNewItem;

	pQueueTmp->cnt++;

	return 0;
}

int Kit_QueueTakeFirst(void *pQueue, void *pItem)
{
	stQueue_t *pQueueTmp = (stQueue_t *)pQueue;
	
	if (pQueueTmp == NULL)
	{
		return -1;
	}

	stQueueItem_t *pFirstItem = pQueueTmp->pHead;
	if (pFirstItem == NULL)
	{
		return -1;
	}

	pQueueTmp->pHead = pQueueTmp->pHead->pNext;
	pQueueTmp->cnt--;

	memcpy(pItem, pFirstItem->pData, pQueueTmp->itemSize);
	Kit_Free(pFirstItem->pData);
	Kit_Free(pFirstItem);

	return 0;
}

int Kit_QueueAt(void *pQueue, int index, void *pItem)
{
	stQueue_t *pQueueTmp = (stQueue_t *)pQueue;
	
	if (pQueueTmp == NULL)
	{
		return -1;
	}

	if (pQueueTmp->cnt <= index)
	{
		return -1;
	}

	stQueueItem_t *pIndexItem = pQueueTmp->pHead;
	if (pIndexItem == NULL)
	{
		return -1;
	}

	for (int i = 0; i < index; i++)
	{
		pIndexItem = pIndexItem->pNext;
		if (pIndexItem == NULL)
		{
			return -1;
		}
	}

	memcpy(pItem, pIndexItem->pData, pQueueTmp->itemSize);

	return 0;
}

int Kit_QueueRemove(void *pQueue, int index)
{
	stQueue_t *pQueueTmp = (stQueue_t *)pQueue;
	
	if (pQueueTmp == NULL)
	{
		return -1;
	}

	if (pQueueTmp->cnt <= index)
	{
		return -1;
	}

	stQueueItem_t *pDeleteItem;
	
	if (index == 0)
	{
		pDeleteItem = pQueueTmp->pHead;
		if (pDeleteItem == NULL)
		{
			return -1;
		}

		pQueueTmp->pHead = pQueueTmp->pHead->pNext;
		pQueueTmp->cnt--;
	}
	else
	{
		stQueueItem_t *pIndexItem = pQueueTmp->pHead;
		if (pIndexItem == NULL)
		{
			return -1;
		}

		for (int i = 0; i < index - 1; i++)
		{
			pIndexItem = pIndexItem->pNext;
			if (pIndexItem == NULL)
			{
				return -1;
			}
		}

		pDeleteItem = pIndexItem->pNext;
		if (pDeleteItem == NULL)
		{
			return -1;
		}

		pIndexItem->pNext = pIndexItem->pNext->pNext;
		pQueueTmp->cnt--;
	}

	Kit_Free(pDeleteItem->pData);
	Kit_Free(pDeleteItem);
	
	return 0;
}

uint32_t Kit_QueueCount(void *pQueue)
{
	stQueue_t *pQueueTmp = (stQueue_t *)pQueue;
	
	if (pQueueTmp == NULL)
	{
		return 0;
	}

	int mdwCnt = pQueueTmp->cnt;

	return mdwCnt;
}

void Kit_QueueClear(void *pQueue)
{
	stQueue_t *pQueueTmp = (stQueue_t *)pQueue;
	
	if (pQueueTmp == NULL)
	{
		return;
	}

	stQueueItem_t *pFirstItem;
	
	while (pQueueTmp->pHead != NULL)
	{
		pFirstItem = pQueueTmp->pHead;
		pQueueTmp->pHead = pQueueTmp->pHead->pNext;
		Kit_Free(pFirstItem->pData);
		Kit_Free(pFirstItem);
	}

	pQueueTmp->cnt = 0;
}


