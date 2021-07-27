/**
 *@file app_subg.c
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
#include "rf69.h"
#include "app_subg.h"
#include "app_ble.h"
#include "kit_delay.h"
#include "kit_log.h"
#include "ocp.h"

#define RF_MODULE_FIFO_SIZE			66
#define WAIT_FIFO_NOT_FULL_TIMEOUT	100//ms
#define TX_TIMEOUT				 	150

#define RX_PAYLAOD_LEN_MINIMED722 	107
#define RX_PAYLAOD_LEN_OMNIPOD		80

#define TX_BUF_SIZE 				255

#define TAG "SUB"

static uint16_t rxPktCnt = 0;
static uint16_t txPktCnt = 0;
static int rxPktRssi = -140;
static bool cmdIntFlag = false;
static eSubgMode_t subgMode = SUBG_MODE_MINIMED_NAS;
static uint8_t txBuf[TX_BUF_SIZE] = {0};
static uint8_t txBufLen;

static uint8_t pktLen;
uint16_t preambleWord;
static uint16_t preambleExtendMs;

static bool wait_fifo_not_full(eRf69Dev_t dev) 
{
	uint16_t cnt;
	
	for(cnt = 0; cnt < WAIT_FIFO_NOT_FULL_TIMEOUT; cnt++) 
	{
		if(!Rf69_IsFifoFull(dev)) 
		{
			return true;
		}
		Kit_DelayMs(1);
	}
	//KIT_LOG(TAG, "Wait fifo not full timeout!");
	
	return false;
}

static void wait_tx_done(eRf69Dev_t dev) 
{
	uint16_t cnt;
	
	for (cnt = 0; cnt < TX_TIMEOUT; cnt++) 
	{
		while(Rf69_IsFifoEmpty(dev))
		{
			return;
		}
		Kit_DelayMs(1);
	}
	//KIT_LOG(TAG, "Wait tx done timeout!");
}

static void minimed_tx(void)
{
	uint16_t txCnt = 0;
	uint8_t zeroByte = 0x00;
	
	Rf69_SetMode(RF69_DEV_FREQ916N868, RF69_MODE_STANDBY);
	Rf69_ClearFifo(RF69_DEV_FREQ916N868);
	
	txCnt = (txBufLen < RF_MODULE_FIFO_SIZE) ? txBufLen : RF_MODULE_FIFO_SIZE;
	Rf69_XmitBuf(RF69_DEV_FREQ916N868, txBuf, txCnt);
	Rf69_SetMode(RF69_DEV_FREQ916N868, RF69_MODE_TX);
		
	while(txCnt < txBufLen) 
	{	
		if(!wait_fifo_not_full(RF69_DEV_FREQ916N868)) 
		{
			return;
		}
		Rf69_XmitByte(RF69_DEV_FREQ916N868, txBuf[txCnt]);
		txCnt++;
	}
	
	if(wait_fifo_not_full(RF69_DEV_FREQ916N868)) 
	{
		Rf69_XmitByte(RF69_DEV_FREQ916N868, zeroByte);
	}
	
	//Rely on the sequencer to end Transmit mode after PacketSent is triggered.
	wait_tx_done(RF69_DEV_FREQ916N868);
}

static void omnipod_tx(void)
{
	bool flag = false;
	uint8_t timeCnt = 0;
	uint8_t txCnt = 0;
	uint8_t txLen = 0;
	uint8_t txBufTmp[255] = {0};
	
	txBufTmp[0] = 0xa5;
	txBufTmp[1] = 0x5a;
	memcpy(txBufTmp + 2, txBuf, txBufLen);
	txBufTmp[txBufLen + 2] = 0xff;
	
	txLen = txBufLen + 3;
	
	Rf69_ClearFifo(RF69_DEV_FREQ433);
	
	while(!Rf69_IsFifoFull(RF69_DEV_FREQ433))
	{
		if(!flag)
		{
			Rf69_XmitByte(RF69_DEV_FREQ433, 0x66);
			flag = true;
		}
		else
		{
			Rf69_XmitByte(RF69_DEV_FREQ433, 0x65);
			flag = false;
		}
	}
	
	Rf69_SetMode(RF69_DEV_FREQ433, RF69_MODE_TX);
	timeCnt = Timer_GetCnt();
	
	while((Timer_GetCnt() - timeCnt) < preambleExtendMs) 
	{
		if(!Rf69_IsFifoOverThreshold(RF69_DEV_FREQ433))
		{
			while(!Rf69_IsFifoFull(RF69_DEV_FREQ433))
			{
				if(!flag)
				{
					Rf69_XmitByte(RF69_DEV_FREQ433, 0x66);
					flag = true;
				}
				else
				{
					Rf69_XmitByte(RF69_DEV_FREQ433, 0x65);
					flag = false;
				}
			}
		}
		Kit_DelayMs(1);
	}
	
	while (txCnt < txLen) 
	{
		if(!Rf69_IsFifoOverThreshold(RF69_DEV_FREQ433))
		{
			while(!Rf69_IsFifoFull(RF69_DEV_FREQ433))
			{
				if((txCnt == 0) && flag)
				{
					Rf69_XmitByte(RF69_DEV_FREQ433, 0x65);
				}
				
				Rf69_XmitByte(RF69_DEV_FREQ433, txBufTmp[txCnt]);
				txCnt++;
			}
		}
		Kit_DelayMs(1);
	}
		
	while(!Rf69_IsFifoEmpty(RF69_DEV_FREQ433));
	{
		Kit_DelayUs(200);
	}
}

static eSubgRxStatus_t minimed_rx(uint8_t *pBuf, uint8_t* pRxLen, uint32_t timeout) 
{	
	uint8_t rxCnt = 0;
	uint8_t rxByteTmp = 0;
	uint32_t timeStart = 0;
	 		
	Rf69_SetMode(RF69_DEV_FREQ916N868, RF69_MODE_STANDBY);
	Rf69_SetPayloadLen(RF69_DEV_FREQ916N868, RX_PAYLAOD_LEN_MINIMED722);
	Rf69_SetMode(RF69_DEV_FREQ916N868, RF69_MODE_RX);
	
	timeStart = Timer_GetCnt();

	while(1)
	{
		if(!Rf69_IsFifoEmpty(RF69_DEV_FREQ916N868))
		{
			rxByteTmp = Rf69_RcvByte(RF69_DEV_FREQ916N868);	
			
			if (rxByteTmp == 0) 
			{
				KIT_LOG(TAG, "Rx byte = 0, break!");
				break;
			}
			
			pBuf[rxCnt++] = rxByteTmp;
		}
		
		if(rxCnt >= RX_PAYLAOD_LEN_MINIMED722)
		{
			KIT_LOG(TAG, "Rx len >= max len, break!");
			break;
		}
	
		if((timeout > 0 && ((Timer_GetCnt() - timeStart) > timeout)) || Ble_GetState() == BLE_STATE_ADV)
		{
			return SUBG_RX_TIMEOUT;
		}

		if(cmdIntFlag)
		{
			return SUBG_RX_INT;
		}
	}
	
	if (rxCnt > 0) 
	{
		// Remove spurious final byte consisting of just one or two high bits.
		uint8_t b = pBuf[rxCnt - 1];
		if (b == 0x80 || b == 0xC0) 
		{
			KIT_LOG(TAG, "End-of-packet glitch 0x%02x.", b >> 6);
			rxCnt--;
		}
	}
	
	if (rxCnt > 0) 
	{
		rxPktCnt++;
		rxPktRssi = Rf69_ReadRssi(RF69_DEV_FREQ916N868, false);
		*pRxLen = rxCnt;
	}
	return SUBG_RX_OK;
}

static eSubgRxStatus_t omnipod_rx(uint8_t *pBuf, uint8_t* pRxLen, uint32_t timeout, uint8_t usePktLen) 
{	
	uint8_t rxCnt = 0;
	uint8_t rxByteTmp = 0;
	uint32_t timeStart = 0;

	Rf69_SetMode(RF69_DEV_FREQ433, RF69_MODE_STANDBY);
	Rf69_SetSyncOnOff(RF69_DEV_FREQ433, true);
	Rf69_SetPayloadLen(RF69_DEV_FREQ433, RX_PAYLAOD_LEN_OMNIPOD);
	Rf69_SetMode(RF69_DEV_FREQ433, RF69_MODE_RX);
	
	timeStart = Timer_GetCnt();

	while(1)
	{
		if(!Rf69_IsFifoEmpty(RF69_DEV_FREQ433))
		{
			rxByteTmp = Rf69_RcvByte(RF69_DEV_FREQ433);	
			
			if ((rxByteTmp >> 6) == 0x03 | (rxByteTmp >> 6) == 0) 
			{
				KIT_LOG(TAG, "Rx byte = 0xf or 0x0, break!");
				break;
			}
			
			pBuf[rxCnt++] = rxByteTmp;
		}
		
		if(rxCnt >= RX_PAYLAOD_LEN_OMNIPOD)
		{
			KIT_LOG(TAG, "Rx len >= max len, break!");
			break;
		}
		
		// Check for end of packet
		if (usePktLen && rxCnt == pktLen) 
		{
			KIT_LOG(TAG, "Rx len = pkt_len, break!");
			break;
		}
				
		if((timeStart > 0 && ((Timer_GetCnt() - timeStart) > timeout)) || Ble_GetState() == BLE_STATE_ADV)
		{
			return SUBG_RX_TIMEOUT;
		}

		if(cmdIntFlag)
		{
			return SUBG_RX_INT;
		}
	}
	
	if (rxCnt > 0) 
	{
		rxPktCnt++;
		rxPktRssi = Rf69_ReadRssi(RF69_DEV_FREQ433, false);
		*pRxLen = rxCnt;
	}

	return SUBG_RX_OK;
}

static void rf_tx_start(void)
{
	txPktCnt++;
	
	switch(subgMode)
	{
		case SUBG_MODE_OMNIPOD:
			omnipod_tx();
			break;
			
		case SUBG_MODE_MINIMED_NAS:
		case SUBG_MODE_MINIMED_WWL:
			minimed_tx();
			break;
			
		default:
			break;
	}
}

static void rf_stop(void)
{
	switch(subgMode)
	{
		case SUBG_MODE_OMNIPOD:
			Rf69_SetMode(RF69_DEV_FREQ433, RF69_MODE_SLEEP);
			break;
			
		case SUBG_MODE_MINIMED_NAS:
		case SUBG_MODE_MINIMED_WWL:
			Rf69_SetMode(RF69_DEV_FREQ916N868, RF69_MODE_SLEEP);
			break;
			
		default:
			break;
	}
}

void Subg_SetMode(eSubgMode_t mode) 
{
	subgMode = mode;	
}

eSubgMode_t Subg_GetMode(void) 
{
	return subgMode;	
}

void Subg_SendPkt(uint8_t *pBuf, uint16_t len, uint8_t repeatCnt, uint16_t repeatIntvl, uint16_t preambleExt) 
{
	uint16_t sendCnt = 0;
	uint16_t totalSendCnt = repeatCnt + 1;
	
	memcpy(txBuf, pBuf, len);
	txBufLen = len;
	preambleExtendMs = preambleExt;
	
	switch(subgMode)
	{
		case SUBG_MODE_OMNIPOD:
			KIT_LOG(TAG, "433 tx setup.");
			Rf69_SetMode(RF69_DEV_FREQ433, RF69_MODE_STANDBY);
			Rf69_SetSyncOnOff(RF69_DEV_FREQ433, false);
			Rf69_SetUnlimitedLenPkt(RF69_DEV_FREQ433);
			Rf69_SetPreambleSize(RF69_DEV_FREQ433, 0);
			break;
			
		case SUBG_MODE_MINIMED_NAS:
			KIT_LOG(TAG, "916 tx setup.");
			Rf69_SetMode(RF69_DEV_FREQ916N868, RF69_MODE_STANDBY);
			Rf69_SetOokBw200khz(RF69_DEV_FREQ916N868);
			Rf69_SetPayloadLen(RF69_DEV_FREQ916N868, len + 1);
			break;
			
		case SUBG_MODE_MINIMED_WWL:
			KIT_LOG(TAG, "868 tx setup.");
			Rf69_SetMode(RF69_DEV_FREQ916N868, RF69_MODE_STANDBY);
			Rf69_SetOokBw250khz(RF69_DEV_FREQ916N868);
			Rf69_SetPayloadLen(RF69_DEV_FREQ916N868, len + 1);
			break;

		default:
			break;
	}
	
	while(sendCnt < totalSendCnt) 
	{
		if(Ble_GetState() == BLE_STATE_ADV)
		{
			break;
		}
		
		if (sendCnt > 0 && repeatIntvl > 0)
		{
			Kit_DelayMs(repeatIntvl);
			wdt_feed(NULL);
		}
		rf_tx_start();
		sendCnt++;
	}
	
	rf_stop();
	
	KIT_LOG(TAG, "Tx done!");
}

eSubgRxStatus_t Subg_GetPkt(uint8_t *pRxBuf, uint8_t *pRxLen, uint32_t timeout, uint8_t usePktLen) 
{
	eSubgRxStatus_t result;
	
	switch(subgMode)
	{
		case SUBG_MODE_OMNIPOD:
			result = omnipod_rx(pRxBuf, pRxLen, timeout, usePktLen);
			break;
			
		case SUBG_MODE_MINIMED_NAS:
		case SUBG_MODE_MINIMED_WWL:
			result = minimed_rx(pRxBuf, pRxLen, timeout);
			break;
			
		default:
			result = SUBG_RX_TIMEOUT;
			break;
	}
	rf_stop();
	
	return result;
}

void Subg_SetFreq(uint32_t freqHz) 
{
	switch(subgMode)
	{
		case SUBG_MODE_OMNIPOD:
			Rf69_SetFreq(RF69_DEV_FREQ433, freqHz);
			break;
			
		case SUBG_MODE_MINIMED_NAS:
			Rf69_SetFreq(RF69_DEV_FREQ916N868, freqHz);
			break;

		case SUBG_MODE_MINIMED_WWL:
			Rf69_SetFreq(RF69_DEV_FREQ916N868, freqHz);//868388000
			break;
			
		default:
			break;
	}
}

void Subg_CfgRf(void)
{	
	switch(subgMode)
	{
		case SUBG_MODE_OMNIPOD:
			Rf69_DevParaCfg(RF69_DEV_FREQ433, RF69_FREQ_433);
			break;
			
		case SUBG_MODE_MINIMED_NAS:
			Rf69_DevParaCfg(RF69_DEV_FREQ916N868, RF69_FREQ_916);
			break;

		case SUBG_MODE_MINIMED_WWL:
			Rf69_DevParaCfg(RF69_DEV_FREQ916N868, RF69_FREQ_868);
			break;
			
		default:
			break;
	}
}

void Subg_Init(void)
{
	Rf69_DevParaCfg(RF69_DEV_FREQ916N868, RF69_FREQ_916);
	Rf69_DevParaCfg(RF69_DEV_FREQ433, RF69_FREQ_433);
}

int Subg_GetRssi(void) 
{
	KIT_LOG(TAG, "Rssi = %d.", rxPktRssi);
	return rxPktRssi;
}

uint16_t Subg_GetRxPktCnt(void) 
{
	return rxPktCnt;
}

uint16_t Subg_GetTxPktCnt(void) 
{
	return txPktCnt;
}

void Subg_SetPreamble(uint16_t preamble) 
{
	preambleWord = preamble;
}

void Subg_SetPktLen(uint8_t len) 
{
	pktLen = len;
}

void Subg_SetIntFlg(void)
{
	cmdIntFlag = true;
}

void Subg_ClrIntFlg(void)
{
	cmdIntFlag = false;
}

/*void Subg_Test(void)
{
	uint8_t data[] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};
	uint8_t i;
	
	subgMode = SUBG_MODE_OMNIPOD;	
	for(i = 0; i < 100; i++)
	{
		Subg_SendPkt(data, 12, 250, 1000, 300);	
	}
}*/



