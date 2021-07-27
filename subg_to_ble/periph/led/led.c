/**
 *@file led.c
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
#include "boards.h"
#include "led.h"
#include "app_timer.h"
#include "nrf_gpio.h"

#define LED_NUMBER		2
#define LED_LIST 		{LED_0_PIN, LED_1_PIN}

#define LED_TIME1    	(30)//ms
#define LED_TIME2    	(300)//ms
#define LED_TIME3		(400)//ms
#define LED_TIME4		(10000)//ms

typedef enum 
{
	LED_OP_BLANK = 0x00,
	LED_OP_YELLOW_LIGHT = 0x01,
	LED_OP_RED_LIGHT = 0x02
}eLedOp_t;

APP_TIMER_DEF(ledTimer);

/***************************************************************
* led control period
****************************************************************/
static const uint32_t RED_LED_TWINKLE_PERIOD[] 		= {LED_TIME1, LED_TIME4};
static const uint32_t YELLOW_LED_TWINKLE_PERIOD[] 	= {LED_TIME1, LED_TIME4};
static const uint32_t LED_CROSS_PERIOD[] 			= {LED_TIME2, LED_TIME2, LED_TIME3};
//must be defined according to the order of "led control period"
static const eLedOp_t RED_LED_TWINKLE_OP[] 		= {LED_OP_RED_LIGHT, LED_OP_BLANK};
static const eLedOp_t YELLOW_LED_TWINKLE_OP[] 	= {LED_OP_YELLOW_LIGHT, LED_OP_BLANK};
static const eLedOp_t LED_CROSS_OP[] 	 		= {LED_OP_RED_LIGHT, LED_OP_YELLOW_LIGHT, LED_OP_BLANK};

static const uint8_t ledList[LED_NUMBER] = LED_LIST;
static const uint16_t ledPolarityByte = 0x00;
static eLedAction_t curLedAction = LED_ACT_NONE;
static uint32_t ledLoopTimes  = 0;
static uint32_t curLedLoopCnt = 0;

/* drive only one led */
static void led_single_control(uint8_t ledId, uint8_t status)
{
    if(status == 1) 
	{
        ((ledPolarityByte >> ledId) & 0x01) ? nrf_gpio_pin_set(ledList[ledId]) : nrf_gpio_pin_clear(ledList[ledId]);
    } 
	else 
	{
        ((ledPolarityByte >> ledId) & 0x01) ? nrf_gpio_pin_clear(ledList[ledId]): nrf_gpio_pin_set(ledList[ledId]);
    }
}

/* use a number to control LED status */
static void led_multiple_control(uint16_t para)
{
    uint8_t i;
	
    for(i = 0; i < LED_NUMBER ; i++) 
	{
        led_single_control(i, ((para >> i) & 0x01));
    }
}

static void led_timer_handle(void * context)
{
    if((curLedLoopCnt >= ledLoopTimes) && ((ledLoopTimes != 0))) 
	{

        curLedAction  = LED_ACT_NONE;
        ledLoopTimes  = 0;
        curLedLoopCnt = 0;
        led_multiple_control(LED_OP_BLANK);
        return;
    }

    eLedAction_t action = * ((eLedAction_t*)context);
    uint32_t timerInterval = 0;
    eLedOp_t op = LED_OP_BLANK;

    switch(action) 
	{
        case LED_ACT_RED_TWINKLE:
            timerInterval = RED_LED_TWINKLE_PERIOD[curLedLoopCnt % (sizeof(RED_LED_TWINKLE_PERIOD) / sizeof(RED_LED_TWINKLE_PERIOD[0]))];
            op = RED_LED_TWINKLE_OP[curLedLoopCnt % (sizeof(RED_LED_TWINKLE_OP) / sizeof(RED_LED_TWINKLE_OP[0]))];
            break;
			
        case LED_ACT_YELLOW_TWINKLE:
            timerInterval = YELLOW_LED_TWINKLE_PERIOD[curLedLoopCnt % (sizeof(YELLOW_LED_TWINKLE_PERIOD) / sizeof(YELLOW_LED_TWINKLE_PERIOD[0]))];
            op = YELLOW_LED_TWINKLE_OP[curLedLoopCnt % (sizeof(YELLOW_LED_TWINKLE_OP) / sizeof(YELLOW_LED_TWINKLE_OP[0]))];
            break;
		
		case LED_ACT_CROSS_TWINKLE:
            timerInterval = LED_CROSS_PERIOD[curLedLoopCnt % (sizeof(LED_CROSS_PERIOD) / sizeof(LED_CROSS_PERIOD[0]))];
            op = LED_CROSS_OP[curLedLoopCnt % (sizeof(LED_CROSS_OP) / sizeof(LED_CROSS_OP[0]))];
			break;
			
        default:
            return;
    }

	led_multiple_control(op);

    app_timer_start(ledTimer, APP_TIMER_TICKS(timerInterval), &curLedAction);
    curLedLoopCnt++;
}

void Led_Init(void)
{
    uint32_t err = NRF_SUCCESS;
    uint32_t i;
	
    for (i = 0; i < LED_NUMBER; i++)
    {
        nrf_gpio_cfg_output(ledList[i]);
    }
	
	led_multiple_control(LED_OP_BLANK);
    err =  app_timer_create(&ledTimer, APP_TIMER_MODE_SINGLE_SHOT, led_timer_handle);
	APP_ERROR_CHECK(err);
}

void Led_Ctrl(eLedAction_t action, uint16_t actionTimes)
{
    app_timer_stop(ledTimer);
	led_multiple_control(LED_OP_BLANK);

    curLedLoopCnt = 0;
    curLedAction = action;

    switch(action) 
	{
        case LED_ACT_NONE:
			led_multiple_control(LED_OP_BLANK);
            break;

		case LED_ACT_RED_LIGHT:
			led_multiple_control(LED_OP_RED_LIGHT);
			break;
			
        case LED_ACT_YELLOW_LIGHT:
			led_multiple_control(LED_OP_YELLOW_LIGHT);
            break;
			
		case LED_ACT_RED_TWINKLE:
            ledLoopTimes = actionTimes * (sizeof(RED_LED_TWINKLE_PERIOD) / sizeof(RED_LED_TWINKLE_PERIOD[0]));
			led_timer_handle(&curLedAction);
			break;
			
		case LED_ACT_YELLOW_TWINKLE:
            ledLoopTimes = actionTimes * (sizeof(YELLOW_LED_TWINKLE_PERIOD) / sizeof(YELLOW_LED_TWINKLE_PERIOD[0]));
			led_timer_handle(&curLedAction);
			break;
			
		case LED_ACT_CROSS_TWINKLE:
            ledLoopTimes = actionTimes * (sizeof(LED_CROSS_PERIOD) / sizeof(LED_CROSS_PERIOD[0]));
			led_timer_handle(&curLedAction);
			break;
			
        default:
            break;
    }
}



