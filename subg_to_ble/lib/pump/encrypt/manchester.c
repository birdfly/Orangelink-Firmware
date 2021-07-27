/**
 *@file manchester.c
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
#include <stdbool.h>

#define MANCHESTER_ONE		0x01
#define MANCHESTER_ZERO		0x02

bool encode_manchester(const uint8_t *src, uint8_t *dst, uint16_t len)
{
	uint16_t i = 0;
	uint8_t j = 0;
	
	for(i = 0; i < len; i++)
	{
		dst[2 * i] = 0;
		dst[2 * i + 1] = 0;
		
		for(j = 0; j < 8; j++)
		{
			if(j < 4)
			{
				dst[2 * i + 1] |= (((src[i] >> j) & 0x01) ? MANCHESTER_ONE : MANCHESTER_ZERO) << (j * 2);
			}
			else
			{
				dst[2 * i] |= (((src[i] >> j) & 0x01) ? MANCHESTER_ONE : MANCHESTER_ZERO) << (j * 2 - 8);
			}
		}
	}
	return true;
}

uint16_t decode_manchester(const uint8_t *src, uint8_t *dst, uint16_t len)
{
	uint16_t i = 0;
	uint8_t j = 0;
	
	for(i=0; i < len / 2; i++)
	{
		dst[i] = 0;
		
		for(j = 0; j < 8; j++)
		{
			if(j < 4)
			{
				if((src[2 * i + 1] >> (j * 2) & 0x03) == MANCHESTER_ZERO)
				{
					dst[i] |= 0x00 << j;
				}
				else if((src[2 * i + 1] >> (j * 2) & 0x03) == MANCHESTER_ONE)
				{
					dst[i] |= 0x01 << j;
				}
				else
				{
					return i;
				}
			}
			else
			{
				if((src[2 * i] >> (j * 2 - 8) & 0x03) == MANCHESTER_ZERO)
				{
					dst[i] |= 0x00 << j;
				}
				else if((src[2 * i] >> (j * 2 - 8) & 0x03) == MANCHESTER_ONE)
				{
					dst[i] |= 0x01 << j;
				}
				else
				{
					return i;
				}
			}
		}
	}
	return i;
}

