/**
 *@file app_battery.c
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
#include <stdbool.h>

#include "nrfx_saadc.h"
#include "kit_delay.h"
#include "kit_log.h"
#include "app_timer.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "app_indication.h"
#include "app_ble.h"

#define BAT_MAX_VOLTAGE 	3200//mv
#define BAT_ADC_MAX_VALUE 	700
#define BAT_LOW_DET_INVL 	180000//ms

#define TAG "BAT"

APP_TIMER_DEF(batDetTimer);
static uint8_t battLevel = 0;
static uint16_t battVoltage = 0;
static const uint16_t voltageTable[] = {3100, 3000, 2900, 2800, 2700, 2600, 2500, 2550, 2400};
static const uint8_t percentageTable[] = {100, 90, 80, 70, 60, 50, 40, 30, 20};//must correspond to voltageTable

static void adc_callback(nrfx_saadc_evt_t const *pEvt)
{
}

static void adc_init(void)
{
	uint32_t err = NRF_SUCCESS;
	
	nrfx_saadc_config_t adcCfg = NRFX_SAADC_DEFAULT_CONFIG;
	err = nrfx_saadc_init(&adcCfg, adc_callback);
    APP_ERROR_CHECK(err);
	
	nrf_saadc_channel_config_t adcChannel = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(ADC_BAT_DET_CHANNEL);
	err = nrfx_saadc_channel_init(ADC_BAT_DET_CHANNEL, &adcChannel);
    APP_ERROR_CHECK(err);
}

static void adc_uninit(void)
{
	nrfx_saadc_uninit();
	nrf_gpio_cfg_default(ADC_BAT_DET_PIN);
}

static uint8_t batt_detect(void)
{
	int16_t adcResult = 0;
	nrf_saadc_value_t adcTmp = 0;
    uint8_t length = sizeof(voltageTable) / sizeof(voltageTable[0]);
	uint8_t i;
		
	adc_init();
	Kit_DelayMs(5);
	for(i = 0; i < 2; i++)
	{
		nrfx_saadc_sample_convert(ADC_BAT_DET_CHANNEL, &adcTmp);
		adcResult += adcTmp;
	}
	adc_uninit();
	adcResult = adcResult / 2;
	battVoltage = (uint16_t)((float)adcResult / BAT_ADC_MAX_VALUE * BAT_MAX_VOLTAGE);
	    
    //find the first value which is < volatage
    for(i = 0; i < length; i++) 
	{
        if(battVoltage > voltageTable[i])
        {
            break;
        }
    }

    if(i >= length) 
	{
        battLevel = 10;
    }
	else
	{
        battLevel = percentageTable[i];
    }
	
	KIT_LOG(TAG, "Level = %d.", battLevel);	

	return battLevel;
}

static void batt_low_check_handle(void *pContext)
{
	static bool flag = false;
	uint8_t batt = 0;
	
	batt = batt_detect();
	if(batt <= 10 && !flag)
	{
		Idc_SetType(INDICATE_LOW_POWER);
		flag = true;
	}
	else if(batt >= 30 && flag)
	{
		if(Ble_GetState() == BLE_STATE_ADV)
		{
			Idc_SetType(INDICATE_ADV);
		}
		else
		{
			Idc_SetType(INDICATE_CONNECTED);
		}
		flag = false;
	}
}

void Batt_Init(void)
{
    uint32_t errCode = NRF_SUCCESS;
	
    errCode =  app_timer_create(&batDetTimer, APP_TIMER_MODE_REPEATED, batt_low_check_handle);
    APP_ERROR_CHECK(errCode);
	
	errCode = app_timer_start(batDetTimer, APP_TIMER_TICKS(BAT_LOW_DET_INVL), NULL);
	APP_ERROR_CHECK(errCode);

	batt_detect();
	
	KIT_LOG(TAG, "Init OK!");
}

uint8_t Batt_GetLevel(void)
{
	return battLevel;
}

uint16_t Batt_GetVoltage(void)
{
	return battVoltage;
}

bool Batt_IsLow(void)
{
	if(battLevel <= 10)
	{
		return true;
	}
	
	return false;
}

