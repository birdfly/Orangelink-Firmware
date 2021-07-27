/**
 *@file ocp.c
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
#include "nrf_log_ctrl.h"
#include "nrf_log.h"
#include "nrf_log_default_backends.h"
#include "nrf_power.h"
#include "nrf_pwr_mgmt.h"
#include "app_timer.h"
#include "nrf_drv_wdt.h"
#include "nrf_drv_timer.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "boards.h"
#include "kit_log.h"

#define TAG "OCP"
/*******************************************************************************
 * flash
 ******************************************************************************/ 
#ifndef USER_DEBUG
const uint32_t flashProtectSet __attribute__((used, at(0x10001208))) = 0xFFFFFF00;
#endif

static void flash_event_handle(nrf_fstorage_evt_t * pEvt)
{
    if (pEvt->result != NRF_SUCCESS)
    {
        KIT_LOG(TAG, "--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (pEvt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            KIT_LOG(TAG, "--> Event received: wrote %d bytes at address 0x%x.",
                        pEvt->len, pEvt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            KIT_LOG(TAG, "--> Event received: erased %d page from address 0x%x.",
                         pEvt->len, pEvt->addr);
        } break;

        default:
            break;
    }
}

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = flash_event_handle,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = FLASH_START_ADDR,
    .end_addr   = FLASH_END_ADDR,
};

static void wait_flash_ready(nrf_fstorage_t const * pFstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(pFstorage))
    {
        nrf_pwr_mgmt_run();
    }
}

void Flash_Read(void* pDest, const uint32_t srcAddr, uint32_t len)
{
    memcpy((uint8_t*)pDest, (uint8_t*)srcAddr, len);
}

void Flash_Write(uint32_t pageAddr, void const * pData, uint32_t len)
{
    uint32_t err = NRF_SUCCESS;

	err = nrf_fstorage_erase(&fstorage, pageAddr, 1, NULL);
	APP_ERROR_CHECK(err);
	wait_flash_ready(&fstorage);

	err = nrf_fstorage_write(&fstorage, pageAddr, pData, len, NULL);
	APP_ERROR_CHECK(err);
	wait_flash_ready(&fstorage);
}

void Flash_Init(void)
{
	uint32_t err = NRF_SUCCESS;
	nrf_fstorage_api_t * pFsApi;

	/* Initialize an fstorage instance using the nrf_fstorage_sd backend.
	 * nrf_fstorage_sd uses the SoftDevice to write to flash. This implementation can safely be
	 * used whenever there is a SoftDevice, regardless of its status (enabled/disabled). */
	pFsApi = &nrf_fstorage_sd;

	err = nrf_fstorage_init(&fstorage, pFsApi, NULL);
	APP_ERROR_CHECK(err);

	/* It is possible to set the start and end addresses of an fstorage instance at runtime.
	 * They can be set multiple times, should it be needed. The helper function below can
	 * be used to determine the last address on the last page of flash memory available to
	 * store data. */
	//(void) nrf5_flash_end_addr_get();
	
	/*
	#ifndef USER_DEBUG
	if((NRF_UICR->APPROTECT & UICR_APPROTECT_PALL_Msk) == (UICR_APPROTECT_PALL_Disabled << UICR_APPROTECT_PALL_Pos)) 
	{
    	NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);
    	
    	while (NRF_NVMC->READY == NVMC_READY_READY_Busy) 
		{
    	}
    	
    	NRF_UICR->APPROTECT = 0xFFFFFF00;
    	
    	while(NRF_NVMC->READY == NVMC_READY_READY_Busy) 
		{
    	}
    	NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    	
    	while(NRF_NVMC->READY == NVMC_READY_READY_Busy) 
		{
    	}
    	
    	NVIC_SystemReset();
  	}
	#endif
	*/
}

/*******************************************************************************
 * watch dog
 ******************************************************************************/ 
static nrf_drv_wdt_channel_id wdtChannelId;

void wdt_feed(void *pContext)
{
	//feed watch dog
	nrf_drv_wdt_channel_feed(wdtChannelId);
}

static void wdt_event_handle(void)
{
	//NOTE: The max amount of time we can spend in WDT interrupt is two cycles of 32768[Hz] clock - after that, reset occurs
}

void Wdt_Init(void)
{
    uint32_t err = NRF_SUCCESS;

	//Configure WDT.
    nrf_drv_wdt_config_t config = NRF_DRV_WDT_DEAFULT_CONFIG;
    err = nrf_drv_wdt_init(&config, wdt_event_handle);
    APP_ERROR_CHECK(err);
    err = nrf_drv_wdt_channel_alloc(&wdtChannelId);
    APP_ERROR_CHECK(err);
    nrf_drv_wdt_enable();
}

/*******************************************************************************
 * power manage
 ******************************************************************************/
void Pwr_MgmtInit(void)
{
    ret_code_t err = NRF_SUCCESS;
	
    err = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err);
}

void Pwr_MgmtIdle(void)
{
    if (false == NRF_LOG_PROCESS())
    {
        nrf_pwr_mgmt_run();
    }
}
 
void Pwr_ResetReason(void)
{
	/* Reset reason */
	uint32_t reason = nrf_power_resetreas_get();
	
	if(0 == reason)
	{
		KIT_LOG(TAG, "Reset: NONE.");	
	}
	
	if(0 != (reason & NRF_POWER_RESETREAS_RESETPIN_MASK))
	{
		KIT_LOG(TAG, "Reset: RESETPIN.");
	}
	
	if(0 != (reason & NRF_POWER_RESETREAS_DOG_MASK	))
	{
		KIT_LOG(TAG, "Reset: DOG.");
	}
	
	if(0 != (reason & NRF_POWER_RESETREAS_SREQ_MASK	))
	{
		KIT_LOG(TAG, "Reset: SREQ.");
	}
	
	if(0 != (reason & NRF_POWER_RESETREAS_LOCKUP_MASK	))
	{
		KIT_LOG(TAG, "Reset: LOCKUP.");
	}
	
	if(0 != (reason & NRF_POWER_RESETREAS_OFF_MASK	))
	{
		KIT_LOG(TAG, "Reset: OFF.");
	}
		
	if(0 != (reason & NRF_POWER_RESETREAS_DIF_MASK	))
	{
		KIT_LOG(TAG, "Reset: DIF.");
	}
	
	nrf_power_resetreas_clear(reason);
}

/*******************************************************************************
 * nrf log
 ******************************************************************************/
uint32_t get_rtc_cnt(void)
{	 
	return NRF_RTC1->COUNTER;
}

void Log_Init(void)
{
	ret_code_t err = NRF_SUCCESS;
	
	err = NRF_LOG_INIT(get_rtc_cnt);
	APP_ERROR_CHECK(err);

	NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/*******************************************************************************
 * timer
 ******************************************************************************/
static const nrf_drv_timer_t timer = NRF_DRV_TIMER_INSTANCE(TIMER_INSTANCE);
static uint32_t totalTimerCnt = 0;
static uint32_t wdtTimerCnt = 0;

//Handler for timer events.
static void timr_evt_handle(nrf_timer_event_t eventType, void* pContext)
{
    switch (eventType)
    {
        case NRF_TIMER_EVENT_COMPARE0:
			totalTimerCnt++;
			wdtTimerCnt++;
			
			if(wdtTimerCnt == WDT_FEED_INTVL)
			{
				wdtTimerCnt = 0;
				wdt_feed(NULL);
			}
            break;

        default:
            //Do nothing.
            break;
    }
}

void Timer_Init(void)
{
    uint32_t timeTicks;
    uint32_t err = NRF_SUCCESS;
	
    //Configure TIMER_LED for generating simple light effect - leds on board will invert his state one after the other.
    nrf_drv_timer_config_t timerCfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
	
    err = nrf_drv_timer_init(&timer, &timerCfg, timr_evt_handle);
    APP_ERROR_CHECK(err);

    timeTicks = nrf_drv_timer_ms_to_ticks(&timer, TIMER_PERIOD_MS);

    nrf_drv_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, timeTicks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    nrf_drv_timer_enable(&timer);

	totalTimerCnt = 0;
}

uint32_t Timer_GetCnt(void)
{
	return totalTimerCnt;
}

/*******************************************************************************
 * rtc
 ******************************************************************************/
void Rtc_Init(void)
{
    ret_code_t err = NRF_SUCCESS;
	
	err = app_timer_init();
    APP_ERROR_CHECK(err);
}

/*******************************************************************************
 * dcdc
 ******************************************************************************/
void Dcdc_Enable(void)//must
{	 
	NRF_POWER->DCDCEN = 1;
}


