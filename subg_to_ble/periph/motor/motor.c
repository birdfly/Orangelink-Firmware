/**
 *@file motor.c
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
#include "motor.h"
#include "nrfx_pwm.h"
#include "app_timer.h"
#include "nrf_gpio.h"

#define MTR_TIME1    	(300)
#define MTR_TIME2    	(600)
#define MTR_TIME3    	(120000)
#define MTR_TIME4       (300000)

#define MTR_PWM_TOP_VALUE    	1000

typedef enum 
{
    MTR_OP_SLIENT = 0,
	MTR_OP_VIBRATE
}eMtrOp_t;

APP_TIMER_DEF(motorTimer);

static nrfx_pwm_t pwmInstance = NRFX_PWM_INSTANCE(PWM_CH);
static nrf_pwm_values_common_t pwmValues;
static nrf_pwm_sequence_t const pwmSeq0 =
{
	.values.p_common	= &pwmValues,
	.length 			= NRF_PWM_VALUES_LENGTH(pwmValues),
	.repeats			= 0,
	.end_delay			= 0,
};

static eMtrAction_t curMotorAction = MTR_ACT_NONE;
static uint32_t motorLoopTimes  = 0;
static uint32_t curMotorLoopCnt = 0;
bool pwmState = false;

/***************************************************************
* Fast and slow motion control period
****************************************************************/
static const uint32_t SHORT_VIBRATE_PERIOD[] = {MTR_TIME1};
static const uint32_t LONG_VIBRATE_PERIOD[] = {MTR_TIME2};
static const uint32_t SERIAL_VIBRATE1_PERIOD[] = {MTR_TIME2, MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4,
	MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4};
static const uint32_t SERIAL_VIBRATE2_PERIOD[] = {MTR_TIME1, MTR_TIME1, MTR_TIME1, MTR_TIME3};
static const uint32_t SERIAL_VIBRATE3_PERIOD[] = {MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4,
	MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME4, MTR_TIME2};
//must be defined according to the order of FAST_MOTION_PERIOD or SLOW_MOTION_PERIOD
static const eMtrOp_t SHORT_VIBRATE_PERIOD_OP[] = {MTR_OP_VIBRATE};
static const eMtrOp_t LONG_VIBRATE_PERIOD_OP[] = {MTR_OP_VIBRATE};
static const eMtrOp_t SERIAL_VIBRATE1_PERIOD_OP[] = {MTR_OP_VIBRATE, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT,
	MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT};
static const eMtrOp_t SERIAL_VIBRATE2_PERIOD_OP[] = {MTR_OP_VIBRATE, MTR_OP_SLIENT, MTR_OP_VIBRATE, MTR_OP_SLIENT};
static const eMtrOp_t SERIAL_VIBRATE3_PERIOD_OP[] = {MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT,
	MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_SLIENT, MTR_OP_VIBRATE};

static void motor_on(void)
{
    uint32_t err = NRF_SUCCESS;
	nrfx_pwm_config_t pwmInstanceCfg = NRFX_PWM_DEFAULT_CONFIG;
	
	pwmInstanceCfg.output_pins[0] = PWM_MOTOR_PIN;
	pwmInstanceCfg.output_pins[1] = NRFX_PWM_PIN_NOT_USED;
	pwmInstanceCfg.output_pins[2] = NRFX_PWM_PIN_NOT_USED;
	pwmInstanceCfg.output_pins[3] = NRFX_PWM_PIN_NOT_USED;
	pwmInstanceCfg.top_value = MTR_PWM_TOP_VALUE;
	if(!pwmState)
	{
		err = nrfx_pwm_init(&pwmInstance, &pwmInstanceCfg, NULL);
	    APP_ERROR_CHECK(err);

		// This array cannot be allocated on stack (hence "static") and it must
		// be in RAM.
		pwmValues = pwmInstanceCfg.top_value / 100 * PWM_MOTOR_DUTY_PCT;
		nrfx_pwm_simple_playback(&pwmInstance, &pwmSeq0, 1, NRFX_PWM_FLAG_LOOP);
		pwmState = true;
	}
}

static void motor_off(void)
{
	if(pwmState)
	{
	    nrfx_pwm_stop(&pwmInstance, true);
		nrfx_pwm_uninit(&pwmInstance);
		pwmState = false;
	}
	nrf_gpio_cfg_default(PWM_MOTOR_PIN);
}

static void motor_timer_handle(void * context)
{
    if((curMotorLoopCnt >= motorLoopTimes) && ((motorLoopTimes != 0))) 
	{
        curMotorAction  = MTR_ACT_NONE;
        motorLoopTimes  = 0;
        curMotorLoopCnt = 0;
        motor_off();
        return;
    }

    eMtrAction_t action = * ((eMtrAction_t*)context);
    uint32_t timerInterval = 0;
    eMtrOp_t op = MTR_OP_SLIENT;

    switch(action) 
	{
        case MTR_ACT_SHORT_VIBRATE:
            timerInterval = SHORT_VIBRATE_PERIOD[curMotorLoopCnt % (sizeof(SHORT_VIBRATE_PERIOD) / sizeof(SHORT_VIBRATE_PERIOD[0]))];
            op = SHORT_VIBRATE_PERIOD_OP[curMotorLoopCnt % (sizeof(SHORT_VIBRATE_PERIOD_OP) / sizeof(SHORT_VIBRATE_PERIOD_OP[0]))];
            break;
			
        case MTR_ACT_LONG_VIBRATE:
            timerInterval = LONG_VIBRATE_PERIOD[curMotorLoopCnt % (sizeof(LONG_VIBRATE_PERIOD) / sizeof(LONG_VIBRATE_PERIOD[0]))];
            op = LONG_VIBRATE_PERIOD_OP[curMotorLoopCnt % (sizeof(LONG_VIBRATE_PERIOD_OP) / sizeof(LONG_VIBRATE_PERIOD_OP[0]))];
            break;
		
		case MTR_ACT_SERIAL_VIBRATE1:
            timerInterval = SERIAL_VIBRATE1_PERIOD[curMotorLoopCnt % (sizeof(SERIAL_VIBRATE1_PERIOD) / sizeof(SERIAL_VIBRATE1_PERIOD[0]))];
            op = SERIAL_VIBRATE1_PERIOD_OP[curMotorLoopCnt % (sizeof(SERIAL_VIBRATE1_PERIOD_OP) / sizeof(SERIAL_VIBRATE1_PERIOD_OP[0]))];
			break;
			
		case MTR_ACT_SERIAL_VIBRATE2:
			timerInterval = SERIAL_VIBRATE2_PERIOD[curMotorLoopCnt % (sizeof(SERIAL_VIBRATE2_PERIOD) / sizeof(SERIAL_VIBRATE2_PERIOD[0]))];
			op = SERIAL_VIBRATE2_PERIOD_OP[curMotorLoopCnt % (sizeof(SERIAL_VIBRATE2_PERIOD_OP) / sizeof(SERIAL_VIBRATE2_PERIOD_OP[0]))];
			break;
			
		case MTR_ACT_SERIAL_VIBRATE3:
			timerInterval = SERIAL_VIBRATE3_PERIOD[curMotorLoopCnt % (sizeof(SERIAL_VIBRATE3_PERIOD) / sizeof(SERIAL_VIBRATE3_PERIOD[0]))];
			op = SERIAL_VIBRATE3_PERIOD_OP[curMotorLoopCnt % (sizeof(SERIAL_VIBRATE3_PERIOD_OP) / sizeof(SERIAL_VIBRATE3_PERIOD_OP[0]))];
			break;
			
        default:
            return;
    }

    if(op == MTR_OP_VIBRATE) 
	{
        motor_on();
    } 
	else 
	{
        motor_off();
    }

    app_timer_start(motorTimer, APP_TIMER_TICKS(timerInterval), &curMotorAction);
    curMotorLoopCnt++;
}

void Motor_Init(void)
{
	uint32_t err = NRF_SUCCESS;	
	
	err =  app_timer_create(&motorTimer, APP_TIMER_MODE_SINGLE_SHOT, motor_timer_handle);
	APP_ERROR_CHECK(err);	
}

void Motor_Ctrl(eMtrAction_t action, uint16_t actionTimes)
{
    //stop the timer already running
    app_timer_stop(motorTimer);
    motor_off();

    curMotorLoopCnt = 0;
    curMotorAction = action;

    switch(action) 
	{
		case MTR_ACT_NONE:
			motor_off();
			break;
			
        case MTR_ACT_SHORT_VIBRATE:
            motorLoopTimes = actionTimes * (sizeof(SHORT_VIBRATE_PERIOD) / sizeof(SHORT_VIBRATE_PERIOD[0]));
			motor_timer_handle(&curMotorAction);
            break;
			
        case MTR_ACT_LONG_VIBRATE:
            motorLoopTimes = actionTimes * (sizeof(LONG_VIBRATE_PERIOD) / sizeof(LONG_VIBRATE_PERIOD[0]));
			motor_timer_handle(&curMotorAction);
            break;

        case MTR_ACT_SERIAL_VIBRATE1:
            motorLoopTimes = actionTimes * (sizeof(SERIAL_VIBRATE1_PERIOD) / sizeof(SERIAL_VIBRATE1_PERIOD[0]));
			motor_timer_handle(&curMotorAction);
            break;

        case MTR_ACT_SERIAL_VIBRATE2:
            motorLoopTimes = actionTimes * (sizeof(SERIAL_VIBRATE2_PERIOD) / sizeof(SERIAL_VIBRATE2_PERIOD[0]));
			motor_timer_handle(&curMotorAction);
            break;
			
		case MTR_ACT_SERIAL_VIBRATE3:
			motorLoopTimes = actionTimes * (sizeof(SERIAL_VIBRATE3_PERIOD_OP) / sizeof(SERIAL_VIBRATE3_PERIOD_OP[0]));
			motor_timer_handle(&curMotorAction);
			break;

        default:
			break;
    }
}




