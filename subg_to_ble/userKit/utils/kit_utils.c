/**
 *@file kit_utils.c
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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

uint32_t Kit_IsSpace(char character)
{
    if(character == ' ' || character == '\t' || character == '\n'
		|| character == '\f' || character == '\b' || character == '\r')
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void Kit_DelChar(char *pStr, char character)
{
    uint32_t i = 0;
	uint32_t j = 0;
	
    while(pStr[i] != '\0')
    {
        if(pStr[i] != character)
        {
            pStr[j++] = pStr[i];
        }
		i++;
    }
	
    pStr[j] = '\0';
}

void Kit_UintToStr(uint32_t uintNum, uint8_t *pStr , uint8_t radix)
{
    char string[] = "0123456789ABCDEFGHIJKLMNOPQLSTUVWXYZ";
    uint8_t *pStrTmp = pStr;
    int remainder = 0;
    int count = -1;
    int i, j;

    while(uintNum >= radix)
    {
        remainder = uintNum % radix;
        uintNum /= radix;
        *pStrTmp++ = string[remainder];
        count++;
    }

    if(uintNum > 0 || (uintNum == 0 && count == -1))
    {
        *pStrTmp++ = string[uintNum];
        count++;
    }

    *pStrTmp = '\0';
    j = count;

    for(i = 0; i < (count + 1) / 2; i++)
    {
        char temp = pStr[i];
        pStr[i] = pStr[j];
        pStr[j--] = temp;
    }
}

int Kit_StrToInt(char *pStr)
{
    int intNum = 0;
    char sign;
	
    if(pStr == NULL) 
    {
		return 0;
    }
	
    while(Kit_IsSpace(*pStr))
    {
        pStr++;
    }
	
	sign = *pStr;

    if(*pStr == '-' || *pStr == '+')
    {
        pStr++;
    }
	
    while(*pStr != '\0')
    {
        if(*pStr < '0' || *pStr > '9')
        {
            break;
        }
        intNum = intNum * 10 + (*pStr -'0');
        pStr++;
    }
	
    if(sign == '-')
    {
        intNum = -intNum;
    }
	
    return intNum;
}

void Kit_HexToStr(uint8_t *pHex, uint8_t *pStr, uint16_t hexLen)
{
	uint32_t i;

	if ((pHex == NULL) || (hexLen == 0) || (pStr == NULL))
	{
		return;
	}

	for(i = hexLen ; i--; pStr += 2)
	{
	    sprintf((char*)pStr, "%02x", *pHex++);
	}
	pStr[hexLen * 2] = '\0';
}

void Kit_StrToHex(uint8_t *pStr, uint8_t *pHex, uint16_t strLen)
{
    uint16_t i;
	
    for(i = 0; i < strLen; i++)
    {
        if(pStr[i] >= 'A'  && pStr[i] <= 'F')
        {
            if((i & 1) == 0)
            {
                pHex[i >> 1] = (pStr[i] - 'A' + 0x0A) << 4;
            }
            else
            {
                pHex[i >> 1] |= pStr[i] - 'A' + 0x0A;
            }
        }
        else if(pStr[i] >= 'a' && pStr[i] <= 'f')
        {
            if((i & 1) == 0)
            {
                pHex[i >> 1] = (pStr[i] - 'a' + 0x0A) << 4;
            }
            else
            {
                pHex[i >> 1] |= pStr[i] - 'a' + 0x0A;
            }
        }
        else if(pStr[i] >= '0' && pStr[i] <= '9')
        {
            if((i & 1) == 0)
            {
                pHex[i >> 1] = (pStr[i] - '0') << 4;
            }
            else
            {
                pHex[i >> 1] |= pStr[i] - '0';
            }
        }
        else
        {
            if((i & 1) == 0)
            {
                pHex[i >> 1] = 0;
            }
            else
            {
                pHex[i >> 1] |= 0;
            }
        }
    }
}

uint16_t Kit_CheckSum16(uint8_t *pBuf, uint8_t len)
{
   uint16_t sum = 0;
   
	while(len > 0) 
	{
		sum = sum + *pBuf;
		pBuf++;
		len--;
	}
	
	return ((~sum) + 1);
}

uint8_t Kit_CheckSum8(uint8_t *pBuf, uint8_t len)
{
   uint8_t sum = 0;
	while(len > 0) 
	{
		sum = sum + *pBuf;
		pBuf++;
		len--;
	}
	return ((~sum) + 1);
}

bool Kit_InsertSeparatorEveryTwoChar(uint8_t *pDestStr, uint8_t *pSrcStr, char sepChar, uint32_t srcLen)
{ 
	uint32_t i;	
	uint32_t len;	

	if(srcLen % 2 != 0)
	{
		return false;
	}
	
	len = srcLen / 2;
	
    for (i = 0; i < len; i++)
    {
		pDestStr[i * 2 + i] = pSrcStr[i * 2];
		pDestStr[i * 2 + i + 1] = pSrcStr[i * 2 + 1];
		
		if(i == len - 1)
		{
			pDestStr[i * 2 + i + 2] = '\0';
		}
		else
		{
			pDestStr[i * 2 + i + 2] = sepChar;
		}
    }
	
	return true;
}

void Kit_BitNegate(uint8_t *pSrc, uint8_t *pDest, uint32_t len)
{
	uint32_t i;	
	
	for(i = 0; i < len; i++)
	{
		pDest[i] = ~pSrc[i];
	}
}

void Kit_SwapBytes(uint8_t *pByte1, uint8_t *pByte2) 
{
	uint8_t tmp = *pByte1;
	
	*pByte1 = *pByte2;
	*pByte2 = tmp;
}

void Kit_ReverseTwoBytes(uint16_t *pWord) 
{
	uint8_t *pTmp = (uint8_t *)pWord;
	
	Kit_SwapBytes(&pTmp[0], &pTmp[1]);
}

void Kit_ReverseFourBytes(uint32_t *pDoubleWord) 
{
	uint8_t *pTmp = (uint8_t *)pDoubleWord;
	
	Kit_SwapBytes(&pTmp[0], &pTmp[3]);
	Kit_SwapBytes(&pTmp[1], &pTmp[2]);
}

