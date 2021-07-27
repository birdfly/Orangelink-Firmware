/**
 *@file bd_xh601_config.h
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
#ifndef __BD_XH601_CONFIG_H__
#define __BD_XH601_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

//LED
#define LED_0_PIN				5
#define LED_1_PIN			    6

//PWM
#define PWM_MOTOR_PIN     		30
#define PWM_CH 	   				0
#define PWM_MOTOR_DUTY_PCT 		50//percentage(0~100)

//battery
#define ADC_BAT_DET_CHANNEL    	NRF_SAADC_INPUT_AIN2
#define ADC_BAT_DET_PIN    		4

//spi
#define SPI_MOSI_PIN	 		15
#define SPI_MISO_PIN	 		16
#define SPI_SCLK_PIN	 		18
#define SPI_NSS_0_PIN			12
#define SPI_NSS_1_PIN			20
#define SPI_INSTANCE			0

//timer
#define TIMER_INSTANCE	 		1
#define TIMER_PERIOD_MS			1

//wdt
#define WDT_FEED_INTVL 			(2000 / TIMER_PERIOD_MS)//ms

//flash
#define FLASH_PAGE_SIZE				4096
#define FLASH_PAGE_NUM				2
#define FLASH_BOOTLOADER_ADDRESS 	(0x00028000UL)
//bootloader start addr:0x28000. if FDS enable, there is 3*4096 size (default)have to keep for FDS,
//so the end addr shuld be (0x00028000 - (4096 * 3) - 1).
#define FLASH_START_ADDR  			(FLASH_BOOTLOADER_ADDRESS - (FLASH_PAGE_SIZE * FLASH_PAGE_NUM))
#define FLASH_END_ADDR    			(FLASH_BOOTLOADER_ADDRESS - 1)
#define FLASH_PAGE_1_ADDR			FLASH_START_ADDR
#define FLASH_PAGE_0_ADDR			(FLASH_PAGE_1_ADDR + FLASH_PAGE_SIZE)

//user config
#define FW_VERSION_MAJOR		0x02
#define FW_VERSION_MINOR		0x05
#define HW_VERSION_MAJOR		0x02
#define HW_VERSION_MINOR		0x05

#define BLE_DEFAULT_NAME  		"Orange"


#ifdef __cplusplus
}
#endif

#endif
