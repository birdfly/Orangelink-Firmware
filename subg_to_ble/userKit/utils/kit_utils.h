/**
 *@file kit_utils.h
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
#ifndef __KIT_UTILS_H__
#define __KIT_UTILS_H__
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t Kit_IsSpace(char character);
void Kit_DelChar(char *pStr, char character);
void Kit_UintToStr(uint32_t num, uint8_t *pStr , uint8_t radix);
int Kit_StrToInt(char *pStr);
void Kit_HexToStr(uint8_t *pHex, uint8_t *pStr, uint16_t hexLen);
void Kit_StrToHex(uint8_t *pStr, uint8_t *pHex, uint16_t strLen);
uint16_t Kit_CheckSum16(uint8_t *pBuf, uint8_t len);
uint8_t Kit_CheckSum8(uint8_t *pBuf, uint8_t len);
bool Kit_InsertSeparatorEveryTwoChar(uint8_t *pDestStr, uint8_t *pSrcStr, char sepChar, uint32_t len);
void Kit_BitNegate(uint8_t *pSrc, uint8_t *pDest, uint32_t len);
void Kit_SwapBytes(uint8_t *pByte1, uint8_t *pByte2);
void Kit_ReverseTwoBytes(uint16_t *pWord);
void Kit_ReverseFourBytes(uint32_t *pDoubleWord);

#ifdef __cplusplus
}
#endif

#endif

