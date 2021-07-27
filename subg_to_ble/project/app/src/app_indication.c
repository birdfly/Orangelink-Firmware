/**
 *@file app_indication.c
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
#include "kit_log.h"
#include "kit_delay.h"
#include "app_timer.h"
#include "app_indication.h"
#include "led.h"
#include "motor.h"
#include "buzzer.h"
#include "app_battery.h"
#include "app_ble.h"
#include "app_config.h"
#include "app_factory.h"

#define STATE_CHECK_TIME 	5000

#if defined(BOARD_XH_5102)
#define MOTOR_TEST_TIME 	220
#endif

#define TAG "IND"

APP_TIMER_DEF(stateCheckTimer);
#if defined(BOARD_XH_5102)
APP_TIMER_DEF(mtrTestTimer);
#endif

static uint8_t curIndication = INDICATE_IDLE;
static uint8_t indicationPriority[] = 
{
	0,	//INDICATE_IDLE
	1,	//INDICATE_ADV
	1,	//INDICATE_CONNECTED
	1,	//INDICATE_DISCONNECTED
	2,	//INDICATE_LOW_POWER
	3,	//INDICATE_NONE_SN
	4,	//INDICATE_FAC_TEST_START
	4	//INDICATE_FAC_TEST_STOP
};

static void state_check_handle(void * pContext)
{
	if(Batt_IsLow())
	{
		Idc_SetType(INDICATE_LOW_POWER);
	}
	else
	{
		if(Ble_GetState() == BLE_STATE_ADV)
		{
			Idc_SetType(INDICATE_ADV);
		}
		else
		{
			Idc_SetType(INDICATE_CONNECTED);
		}
	}
}

#if defined(BOARD_XH_5102)
static void motor_test_handle(void * pContext)
{
	Motor_Ctrl(MTR_ACT_LONG_VIBRATE, 1);
}
#endif

void Idc_Init(void)
{
	uint32_t err = NRF_SUCCESS;	
		
	Motor_Init();
	Led_Init();
	
	#if defined(BOARD_XH_5102)
	Buzzer_Init();
	#endif
	
	Kit_DelayMs(100);
	Led_Ctrl(LED_ACT_CROSS_TWINKLE, 1);

	
	#if defined(BOARD_XH_5102)
	Buzzer_Ctrl(BUZ_ACT_SHORT_RING, 1);
	err =  app_timer_create(&mtrTestTimer, APP_TIMER_MODE_SINGLE_SHOT, motor_test_handle);
	APP_ERROR_CHECK(err);	
    err =  app_timer_start(mtrTestTimer, APP_TIMER_TICKS(MOTOR_TEST_TIME), NULL);
	APP_ERROR_CHECK(err);	
	#else
	Motor_Ctrl(MTR_ACT_LONG_VIBRATE, 1);
	#endif
	
	err =  app_timer_create(&stateCheckTimer, APP_TIMER_MODE_SINGLE_SHOT, state_check_handle);
	APP_ERROR_CHECK(err);	
    err =  app_timer_start(stateCheckTimer, APP_TIMER_TICKS(STATE_CHECK_TIME), NULL);
	APP_ERROR_CHECK(err);	
	
	KIT_LOG(TAG, "Init OK!");	
}

void Idc_SetType(eIndicationType_t type)
{
    if((indicationPriority[type] < indicationPriority[curIndication]) || (type == curIndication))
    {
        return;
    }
	else
	{
		curIndication = type;
	}
	
    switch(type) 
	{
        case INDICATE_FAC_TEST_START:
			Led_Ctrl(LED_ACT_NONE, 0);
			Motor_Ctrl(MTR_ACT_NONE, 0);
            break;
			
		case INDICATE_FAC_TEST_STOP:
			Led_Ctrl(LED_ACT_NONE, 0);
			Motor_Ctrl(MTR_ACT_NONE, 0);
			curIndication = INDICATE_IDLE;
			break;
				
        case INDICATE_NONE_SN:
            Led_Ctrl(LED_ACT_YELLOW_LIGHT, 0);
            break;
				
        case INDICATE_LOW_POWER:
            Led_Ctrl(LED_ACT_RED_LIGHT, 0);
			Motor_Ctrl(MTR_ACT_SERIAL_VIBRATE2, 0);
            break;
			
        case INDICATE_DISCONNECTED:
			if(Cfg_GetMotionData0() == 1)
			{
	            Led_Ctrl(LED_ACT_RED_TWINKLE, 0);
			}
			else
			{
            	Led_Ctrl(LED_ACT_NONE, 0);
			}
			
			if(Cfg_GetMotionData1() == 1)
			{
				Motor_Ctrl(MTR_ACT_SERIAL_VIBRATE1, 0);
			}
			else
			{
				Motor_Ctrl(MTR_ACT_NONE, 0);
			}
            break;
			
        case INDICATE_CONNECTED:
			if(Cfg_GetMotionData0() == 1)
			{
            	Led_Ctrl(LED_ACT_YELLOW_TWINKLE, 0);
			}
			else
			{
            	Led_Ctrl(LED_ACT_NONE, 0);
			}
			
			if(Cfg_GetMotionData1() == 1)
			{
				Motor_Ctrl(MTR_ACT_SHORT_VIBRATE, 1);
			}
			else
			{
				Motor_Ctrl(MTR_ACT_NONE, 0);
			}
            break;
            
		case INDICATE_ADV:
			if(Cfg_GetMotionData0() == 1)
			{
				Led_Ctrl(LED_ACT_RED_TWINKLE, 0);
			}
			else
			{
            	Led_Ctrl(LED_ACT_NONE, 0);
			}
			
			if(Cfg_GetMotionData1() == 1)
			{
				Motor_Ctrl(MTR_ACT_SERIAL_VIBRATE3, 0);
			}
			else
			{
				Motor_Ctrl(MTR_ACT_NONE, 0);
			}
			break;
				
		case INDICATE_IDLE:
			Led_Ctrl(LED_ACT_NONE, 0);
			Motor_Ctrl(MTR_ACT_NONE, 0);
			break;

        default:
			curIndication = type;
            break;
    }
}




