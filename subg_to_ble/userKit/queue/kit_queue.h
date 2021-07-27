/**
 *@file kit_queue.h
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
#ifndef __KIT_QUEUE_H__
#define __KIT_QUEUE_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *Kit_QueueCreate(uint16_t itemSize);
void Kit_QueueDestroy(void **ppQueue);
int Kit_QueueAppend(void *pQueue, void *pItem);
int Kit_QueueTakeFirst(void *pQueue, void *pItem);
int Kit_QueueAt(void *pQueue, int index, void *pItem);
int Kit_QueueRemove(void *pQueue, int index);
uint32_t Kit_QueueCount(void *pQueue);
void Kit_QueueClear(void *pQueue);

#ifdef __cplusplus
}
#endif

#endif 
