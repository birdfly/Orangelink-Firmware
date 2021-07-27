/**
 *@file kit_assert.h
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
#ifndef __KIT_ASSERT_H__
#define __KIT_ASSERT_H__
#include "kit_config.h"
#include "kit_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef KIT_ASSERT_SUPPORT
#define KIT_ASSERT(expr)                                                  \
if(expr)                                                                  \
{                                                                         \
}                                                                         \
else                                                                      \
{                                                                         \
	KIT_LOG_RAW("Error @ file:%s,line\r\n", __FILE__, __LINE__);          \
	while(1);                                                            \
}
#else
#define KIT_ASSERT(expr)
#endif

#ifdef __cplusplus
}
#endif

#endif
