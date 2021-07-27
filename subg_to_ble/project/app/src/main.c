/**
 *@file main.c
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
#include "app_ble.h"
#include "app_battery.h"
#include "app_aps.h"
#include "app_indication.h"
#include "app_sys.h"
#include "app_subg.h"
#include "app_factory.h"
#include "app_config.h"

int main(void)
{
    Sys_Init();
	Cfg_Init();
	Aps_Init();
	Ble_Init();
	Idc_Init();
	Batt_Init();
	Subg_Init();
	Fct_Init();

    for(;;)
    {
        Sys_Idle();
    }
}

