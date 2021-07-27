/**
 *@file manchester.h
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
#ifndef __MANCHESTER_H__
#define __MANCHESTER_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool encode_manchester(const uint8_t *src, uint8_t *dst, uint16_t len);
uint16_t decode_manchester(const uint8_t *src, uint8_t *dst, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __MANCHESTER_H__ */

