/**
 *@file boards.h
 *@author Ribin Huang (you@domain.com)
 *@brief 
 *@version 1.0
 *@date 2020-12-22
 *
 *Copyright (c) 2019 - 2020 Fractal Auto Technology Co.,Ltd.
 *All right reserved.
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License version 2 as
 *published by the Free Software Foundation.
 *
 */
#ifndef __BOARDS_H__
#define __BOARDS_H__

#if defined(BOARD_XH601)
#include "bd_xh601_config.h"
#elif defined(BOARD_XH_5102)
#include "bd_xh_5102_config.h"
#else
#error "Board is not defined"
#endif

#endif
