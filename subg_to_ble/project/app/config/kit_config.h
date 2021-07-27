/**
 *@file kit_config.h
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
#ifndef __KIT_CONFIG_H__
#define __KIT_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Memory size in heap */
#define HEAP_SIZE                               (10 * 1024)

/* Memory block size of heap */
#define HEAP_BLOCK_SIZE                         (32)  	  						

/* Memory block table size */
#define HEAP_ALLOC_TABLE_SIZE                   (HEAP_SIZE / HEAP_BLOCK_SIZE) 	

//#define KIT_ASSERT_SUPPORT

#ifdef USER_DEBUG
#define KIT_LOG_SUPORT
#endif

#ifdef __cplusplus
}
#endif

#endif
