/**
 *@file kit_log.h
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
#ifndef __KIT_LOG_H__
#define __KIT_LOG_H__
#include <stdint.h>

#include "kit_config.h"
#include "nrf_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef KIT_LOG_SUPORT
#define KIT_LOG_RAW 				  NRF_LOG_RAW_INFO
#define KIT_LOG(moduleName, fmt, ...) NRF_LOG_RAW_INFO("[%s] "fmt"\r\n", moduleName, ##__VA_ARGS__)//NRF_LOG_DEBUG("[%s] "fmt, moduleName, ##__VA_ARGS__)										
#else
#define KIT_LOG_RAW(fmt, ...)
#define KIT_LOG(moduleName, fmt, ...)
#endif

void Kit_PrintBytes(const char *pTag, const char *pMsg, const uint8_t *pBuf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif

