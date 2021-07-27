/**
 *@file buzzer.c
 *@author Ribin Huang (you@domain.com)
 *@brief 
 *@version 1.0
 *@date 2021-05-20
 *
 *Copyright (c) 2019 - 2020 Fractal Auto Technology Co.,Ltd.
 *All right reserved.
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License version 2 as
 *published by the Free Software Foundation.
 *
 */
#if defined(BOARD_XH_5102)	
#include "boards.h"
#include "buzzer.h"
#include "nrfx_pwm.h"
#include "app_timer.h"
#include "nrf_gpio.h"

#define BUZ_TIME1    	(200)
#define BUZ_TIME2    	(500)
#define BUZ_TIME3    	(2000)

#define BUZ_PWM_TOP_VALUE    	366

typedef enum 
{
    BUZ_OP_SLIENT = 0,
	BUZ_OP_RING
}eBuzOp_t;

APP_TIMER_DEF(buzzerTimer);

static nrfx_pwm_t pwmInstance = NRFX_PWM_INSTANCE(PWM_CH);
static nrf_pwm_values_common_t pwmValues;
static nrf_pwm_sequence_t const pwmSeq0 =
{
	.values.p_common	= &pwmValues,
	.length 			= NRF_PWM_VALUES_LENGTH(pwmValues),
	.repeats			= 0,
	.end_delay			= 0,
};

static eBuzAction_t curBuzzerAction = BUZ_ACT_NONE;
static uint32_t buzzerLoopTimes  = 0;
static uint32_t curBuzzerLoopCnt = 0;
extern bool pwmState;

/***************************************************************
* motion control period
****************************************************************/
static const uint32_t SHORT_RING_PERIOD[] = {BUZ_TIME1};
static const uint32_t LONG_RING_PERIOD[] = {BUZ_TIME2};
static const uint32_t SERIAL_RING_PERIOD[] = {BUZ_TIME1, BUZ_TIME1, BUZ_TIME1};

//must be defined according to the order of period
static const eBuzOp_t SHORT_RING_PERIOD_OP[] = {BUZ_OP_RING};
static const eBuzOp_t LONG_RING_PERIOD_OP[] = {BUZ_OP_RING};
static const eBuzOp_t SERIAL_RING_PERIOD_OP[] = {BUZ_OP_RING, BUZ_OP_SLIENT, BUZ_OP_RING};

static void buzzer_on(void)
{
    uint32_t err = NRF_SUCCESS;
	nrfx_pwm_config_t pwmInstanceCfg = NRFX_PWM_DEFAULT_CONFIG;
	
	pwmInstanceCfg.output_pins[0] = PWM_BUZZER_PIN;
	pwmInstanceCfg.output_pins[1] = NRFX_PWM_PIN_NOT_USED;
	pwmInstanceCfg.output_pins[2] = NRFX_PWM_PIN_NOT_USED;
	pwmInstanceCfg.output_pins[3] = NRFX_PWM_PIN_NOT_USED;
	pwmInstanceCfg.top_value = BUZ_PWM_TOP_VALUE;
	if(!pwmState)
	{
		err = nrfx_pwm_init(&pwmInstance, &pwmInstanceCfg, NULL);
	    APP_ERROR_CHECK(err);

		// This array cannot be allocated on stack (hence "static") and it must
		// be in RAM.
		pwmValues = pwmInstanceCfg.top_value / 100 * PWM_BUZZER_DUTY_PCT;
		nrfx_pwm_simple_playback(&pwmInstance, &pwmSeq0, 1, NRFX_PWM_FLAG_LOOP);
		pwmState = true;
	}
}

static void buzzer_off(void)
{
	if(pwmState)
	{
	    nrfx_pwm_stop(&pwmInstance, true);
		nrfx_pwm_uninit(&pwmInstance);
		pwmState = false;
	}
	nrf_gpio_cfg_default(PWM_BUZZER_PIN);
}

static void buzzer_timer_handle(void * context)
{
    if((curBuzzerLoopCnt >= buzzerLoopTimes) && ((buzzerLoopTimes != 0))) 
	{
        curBuzzerAction  = BUZ_ACT_NONE;
        buzzerLoopTimes  = 0;
        curBuzzerLoopCnt = 0;
        buzzer_off();
        return;
    }

    eBuzAction_t action = * ((eBuzAction_t*)context);
    uint32_t timerInterval = 0;
    eBuzOp_t op = BUZ_OP_SLIENT;

    switch(action) 
	{
        case BUZ_ACT_SHORT_RING:
            timerInterval = SHORT_RING_PERIOD[curBuzzerLoopCnt % (sizeof(SHORT_RING_PERIOD) / sizeof(SHORT_RING_PERIOD[0]))];
            op = SHORT_RING_PERIOD_OP[curBuzzerLoopCnt % (sizeof(SHORT_RING_PERIOD_OP) / sizeof(SHORT_RING_PERIOD_OP[0]))];
            break;
		
		case BUZ_ACT_LONG_RING:
            timerInterval = LONG_RING_PERIOD[curBuzzerLoopCnt % (sizeof(LONG_RING_PERIOD) / sizeof(LONG_RING_PERIOD[0]))];
            op = LONG_RING_PERIOD_OP[curBuzzerLoopCnt % (sizeof(LONG_RING_PERIOD_OP) / sizeof(LONG_RING_PERIOD_OP[0]))];
			break;
			
		case BUZ_ACT_SERIAL_RING:
			timerInterval = SERIAL_RING_PERIOD[curBuzzerLoopCnt % (sizeof(SERIAL_RING_PERIOD) / sizeof(SERIAL_RING_PERIOD[0]))];
			op = SERIAL_RING_PERIOD_OP[curBuzzerLoopCnt % (sizeof(SERIAL_RING_PERIOD_OP) / sizeof(SERIAL_RING_PERIOD_OP[0]))];
			break;
			
        default:
            return;
    }

    if(op == BUZ_OP_RING) 
	{
        buzzer_on();
    } 
	else 
	{
        buzzer_off();
    }

    app_timer_start(buzzerTimer, APP_TIMER_TICKS(timerInterval), &curBuzzerAction);
    curBuzzerLoopCnt++;
}

void Buzzer_Init(void)
{
	uint32_t err = NRF_SUCCESS;	
	
	err =  app_timer_create(&buzzerTimer, APP_TIMER_MODE_SINGLE_SHOT, buzzer_timer_handle);
	APP_ERROR_CHECK(err);	
}

void Buzzer_Ctrl(eBuzAction_t action, uint16_t actionTimes)
{
    //stop the timer already running
    app_timer_stop(buzzerTimer);
    buzzer_off();

    curBuzzerLoopCnt = 0;
    curBuzzerAction = action;

    switch(action) 
	{
		case BUZ_ACT_NONE:
			buzzer_off();
			break;
			
        case BUZ_ACT_SHORT_RING:
            buzzerLoopTimes = actionTimes * (sizeof(SHORT_RING_PERIOD) / sizeof(SHORT_RING_PERIOD[0]));
			buzzer_timer_handle(&curBuzzerAction);
            break;
			
        case BUZ_ACT_LONG_RING:
            buzzerLoopTimes = actionTimes * (sizeof(LONG_RING_PERIOD) / sizeof(LONG_RING_PERIOD[0]));
			buzzer_timer_handle(&curBuzzerAction);
            break;

        case BUZ_ACT_SERIAL_RING:
            buzzerLoopTimes = actionTimes * (sizeof(SERIAL_RING_PERIOD) / sizeof(SERIAL_RING_PERIOD[0]));
			buzzer_timer_handle(&curBuzzerAction);
            break;

        default:
			break;
    }
}
#endif

