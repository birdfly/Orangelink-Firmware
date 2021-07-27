/**
 *@file app_aps.c
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
#include "app_timer.h"
#include "kit_fifo.h"
#include "kit_log.h"
#include "app_ble.h"
#include "kit_delay.h"
#include "kit_utils.h"
#include "app_subg.h"
#include "4b6b.h"
#include "manchester.h"

#define RILEY_LINK_FXOSC	(24000000)

#define MAX_868_FREQ		(870000000)
#define MIN_868_FREQ		(866000000)
#define MAX_916_FREQ		(918000000)
#define MIN_916_FREQ		(914000000)
#define MAX_433_FREQ		(435000000)
#define MIN_433_FREQ		(431000000)

#define SUBG_STATE_OK 		"OK"
#define SUBG_SW_VER  		"subg_rfspy 2.2"

#define APS_MAX_PARA_LEN				16
#define SUBG_MAX_PKT_LEN				107// 4bit->6bit:(71*4+71*2)/4=106.5(71-byte long packet),and 433 max unencode len is 80 bytes

#define APS_CMD_LOOP_TIME_MS   			10
#define APS_CMD_QUEUE_SIZE				2// 2 = actually only one

#define BLE_RESPONSE_MAX_LEN			150

#define RESPONSE_CODE_RX_TIMEOUT 		0xaa
#define RESPONSE_CODE_CMD_INTERRUPTED 	0xbb
#define RESPONSE_CODE_SUCCESS 			0xdd
#define RESPONSE_CODE_PARAM_ERROR 		0x11
#define RESPONSE_CODE_UNKNOWN_COMMAND 	0x22

#define TAG		"APS"

APP_TIMER_DEF(apsCmdLoopTimer);

typedef enum
{
	CMD_GET_STATE       = 0x01,
	CMD_GET_VER         = 0x02,
	CMD_GET_PKT         = 0x03,
	CMD_SEND_PKT        = 0x04,
	CMD_SEND_AND_LISTEN = 0x05,
	CMD_UPDATE_REG      = 0x06,
	CMD_RESET           = 0x07,
	CMD_LED             = 0x08,
	CMD_READ_REG        = 0x09,
	CMD_SET_MODE_REG    = 0x0a,
	CMD_SET_SW_ENCODING = 0x0b,
	CMD_SET_PREAMBLE    = 0x0c,
	CMD_RESET_RADIO_CFG = 0x0d,
	CMD_GET_STATISTICS  = 0x0e
}eCmdTypes_t;

typedef enum 
{
	ENCRYPT_NONE = 0,
	ENCRYPT_MANCHESTER = 1,
	ENCRYPT_4B6B = 2,
}eEncryptType_t;

typedef struct 
{
	eCmdTypes_t	cmd;
	int8_t 		rssi;
	uint16_t 	pktLen;
	uint8_t 	pkt[APS_MAX_PARA_LEN + SUBG_MAX_PKT_LEN];//tow types data:raw data or encoded data
}stApsReqPkt_t;

typedef struct __attribute__((packed)) 
{
	uint8_t rssi;
	uint8_t pktCnt;
	uint8_t pkt[SUBG_MAX_PKT_LEN];
}stSubgRespPkt_t;

typedef struct __attribute__((packed)) 
{
	uint8_t  listenChan;
	uint32_t listenTimeout;
}stCmdGetPkt_t;

typedef struct __attribute__((packed)) 
{
	uint8_t  sendChan;
	uint8_t  repeatCnt;
	uint16_t repeatIntvl;
	uint16_t preambleExtend;
	uint8_t  sendPkt[];
}stCmdSendPkt_t;

typedef struct __attribute__((packed)) 
{
	uint8_t  sendChan;
	uint8_t  repeatCnt;
	uint16_t repeatIntvl;
	uint8_t  listenChan;
	uint32_t listenTimeout;
	uint8_t  retryCnt;
	uint16_t preambleExtend;
	uint8_t  sendPkt[];
}stCmdSendAndListen_t;

typedef struct __attribute__((packed)) 
{
	uint32_t  updTime;
	uint16_t  rxOverflowCnt;
	uint16_t  rxFifoOverflowCnt;
	uint16_t  pktRxCnt;
	uint16_t  pktTxCnt;
	uint16_t  crcFailCnt;
	uint16_t  spiSyncFailCnt;
	uint16_t  placeholder0;
	uint16_t  placeholder1;
}stCmdGetStatisticsRespPkt_t;

static stKitFifoStruct_t apsCmdQueue;
static stApsReqPkt_t apsCmdBuf[APS_CMD_QUEUE_SIZE];
static uint32_t apsCmdLoopCnt = 0;
static uint8_t subgFreqReg[3] = {0x12, 0x14, 0x83};
static uint8_t usePktLen = 0;
static eEncryptType_t encryptType = ENCRYPT_NONE;
static bool apsLoopStart = false;

static bool encrypt_set(eEncryptType_t type) 
{
	switch(type) 
	{
		case ENCRYPT_NONE:
		case ENCRYPT_MANCHESTER:
		case ENCRYPT_4B6B:
			encryptType = type;
			//KIT_LOG(TAG, "Encrypt set: 0x%02X.", encryptType);
			break;
			
		default:
			return false;
	}
	return true;
}

static uint16_t encrypt_encode(uint8_t *pSrc, uint8_t *pDst, uint16_t len) 
{
	uint16_t encodeLen = 0;
	
	switch(encryptType) 
	{
		case ENCRYPT_NONE:
			//KIT_LOG(TAG, "Encode type: NONE.");
			memcpy(pDst, pSrc, len);
			encodeLen = len;
			break;
			
		case ENCRYPT_MANCHESTER:
			//KIT_LOG(TAG, "Encode type: MANCHESTER.");
			if(encode_manchester(pSrc, pDst, len))
			{
				encodeLen = len * 2;
			}
			break;

		case ENCRYPT_4B6B:
			//KIT_LOG(TAG, "Encode type: 4B6B.");
			encodeLen = encode_4b6b(pSrc, pDst, len);			
			break;
			
		default:
			//KIT_LOG(TAG, "Encode: unknown encode type 0x%02X.", encryptType);
			break;
	}
	
	//KIT_LOG(TAG, "Encode len: %d.", encodeLen);
	
	return encodeLen;
}


uint16_t encrypt_decode(uint8_t *pSrc, uint8_t *pDst, uint16_t len) 
{
	uint16_t decodeLen = 0;

	switch(encryptType) 
	{
		case ENCRYPT_NONE:
			//KIT_LOG(TAG, "Decode type: NONE.");
			memcpy(pDst, pSrc, len);
			decodeLen = len;
			break;
		
		case ENCRYPT_MANCHESTER:
			//KIT_LOG(TAG, "Decode type: MANCHESTER.");			
			decodeLen = decode_manchester(pSrc, pDst, len);
			break;

		case ENCRYPT_4B6B:
			//KIT_LOG(TAG, "Decode type: 4B6B.");
			decodeLen = decode_4b6b(pSrc, pDst, len);
			break;
			
		default:
			//KIT_LOG(TAG, "Decode: unknown decode type 0x%02X.", encryptType);
			break;
	}
	
	//KIT_LOG(TAG, "Decode len: %d.", decodeLen);
	
	return decodeLen;
}

static void send_byte_to_ble(const uint8_t byte) 
{
	uint8_t sendByte;

	sendByte = byte;
	Ble_IpsNotifyRespCntAndSendData(&sendByte, 1);
}

static void send_bytes_to_ble(const uint8_t *pBytes, int len) 
{
	uint8_t sendBytes[BLE_RESPONSE_MAX_LEN] = {0};
	uint16_t sendLen = 0;
		
	sendBytes[0] = RESPONSE_CODE_SUCCESS;
	memcpy(sendBytes + 1, pBytes, len);
	sendLen = len + 1;
	Ble_IpsNotifyRespCntAndSendData(sendBytes, sendLen);
}

static uint8_t  convert_rssi_to_cc111x(int rssi) 
{
	uint8_t cc111xRssi;
	int offset = 73;
	
	cc111xRssi = (uint8_t)((rssi + offset) * 2);
	
	return cc111xRssi;
}

static bool valid_freq_and_set_mode(uint32_t freq) 
{
	if (MIN_868_FREQ <= freq && freq <= MAX_868_FREQ) 
	{
		Subg_SetMode(SUBG_MODE_MINIMED_WWL);
		//KIT_LOG(TAG, "Set subg mode 868.");
		return true;
	}
	else if (MIN_916_FREQ <= freq && freq <= MAX_916_FREQ) 
	{
		Subg_SetMode(SUBG_MODE_MINIMED_NAS);
		//KIT_LOG(TAG, "Set subg mode 916.");
		return true;
	}
	else if (MIN_433_FREQ <= freq && freq <= MAX_433_FREQ) 
	{
		Subg_SetMode(SUBG_MODE_OMNIPOD);
		//KIT_LOG(TAG, "Set subg mode 433.");
		return true;
	}
	return false;
}

static void check_and_set_freq(void) 
{
	uint32_t regValue = ((uint32_t)subgFreqReg[0] << 16) + ((uint32_t)subgFreqReg[1] << 8) + ((uint32_t)subgFreqReg[2]);
	uint32_t freq = (uint32_t)(((uint64_t)regValue * RILEY_LINK_FXOSC) >> 16);
	
	if(valid_freq_and_set_mode(freq)) 
	{
		KIT_LOG(TAG, "Set freq to: %dHz.", freq);
		Subg_SetFreq(freq);
	} 
	else 
	{
		KIT_LOG(TAG, "Invalid freq: %dHz.", freq);
	}
}

static void cmd_set_sw_encoding(const uint8_t *buf, uint16_t len) 
{
	if(encrypt_set((eEncryptType_t)buf[0]))
	{
		send_byte_to_ble(RESPONSE_CODE_SUCCESS);
	}
	else
	{
		send_byte_to_ble(RESPONSE_CODE_PARAM_ERROR);
	}
}

static void cmd_set_mode_reg(void) 
{
	send_byte_to_ble(RESPONSE_CODE_SUCCESS);
}

static void cmd_get_pkt(const uint8_t *pBuf) 
{
	uint8_t getPkt[SUBG_MAX_PKT_LEN] = {0};
	uint8_t getPktLen = 0;
	eSubgRxStatus_t result;
	
	stSubgRespPkt_t subgResp;
	uint8_t decodePkt[SUBG_MAX_PKT_LEN] = {0};
	uint16_t decodePktLen = 0;

	stCmdGetPkt_t *p = (stCmdGetPkt_t *)pBuf;
	Kit_ReverseFourBytes((uint32_t *)&p->listenTimeout);
	KIT_LOG(TAG, "Listen timeout: %d.", p->listenTimeout);
	
	result = Subg_GetPkt(getPkt, &getPktLen, p->listenTimeout, usePktLen);
	
	switch(result)
	{
		case SUBG_RX_OK:		
			subgResp.rssi = convert_rssi_to_cc111x(Subg_GetRssi());
			subgResp.pktCnt = (uint8_t)(Subg_GetRxPktCnt() & 0xff);
			Kit_PrintBytes(TAG, "Rx done:", (const uint8_t*)getPkt, getPktLen);
			decodePktLen = encrypt_decode(getPkt, decodePkt, getPktLen);
			memcpy(subgResp.pkt, decodePkt, decodePktLen);
			send_bytes_to_ble((const uint8_t *)&subgResp, 2 + decodePktLen);
			break;
			
		case SUBG_RX_TIMEOUT:
			KIT_LOG(TAG, "Resp: rx timeout!");
			send_byte_to_ble(RESPONSE_CODE_RX_TIMEOUT);
			break;

		case SUBG_RX_INT:
			KIT_LOG(TAG, "Resp: rx interrupted!");
			send_byte_to_ble(RESPONSE_CODE_CMD_INTERRUPTED);
			break;
			
		default:
			break;
	}
 }

static void cmd_get_state(void) 
{
	send_bytes_to_ble((const uint8_t *)SUBG_STATE_OK, strlen(SUBG_STATE_OK));
}

static void cmd_get_version(void) 
{
	send_bytes_to_ble((const uint8_t *)SUBG_SW_VER, strlen(SUBG_SW_VER));
}
 
static void cmd_send_pkt(const uint8_t *pBuf, uint16_t len) 
{
	uint16_t sendPktLen = 0;
	uint8_t encodePkt[SUBG_MAX_PKT_LEN] = {0};
	uint8_t encodePktLen = 0;
	
	stCmdSendPkt_t *p = (stCmdSendPkt_t *)pBuf;

	Kit_ReverseTwoBytes((uint16_t *)&p->repeatIntvl);
	Kit_ReverseTwoBytes((uint16_t *)&p->preambleExtend);
	KIT_LOG(TAG, "Len: %d, repeat cnt: %d, repeat intvl: %d, preamble extend: %d.", 
		len, p->repeatCnt, p->repeatIntvl, p->preambleExtend);
	
	sendPktLen = len - (p->sendPkt - (uint8_t *)p);
	
	if ((sendPktLen > 0 && p->sendPkt[sendPktLen - 1] == 0) && (Subg_GetMode() != SUBG_MODE_OMNIPOD))
	{
		sendPktLen--;
		KIT_LOG(TAG, "Last byte is 0, len - 1.");
	}

	encodePktLen = encrypt_encode(p->sendPkt, encodePkt, sendPktLen);
	Subg_SendPkt(encodePkt, encodePktLen, p->repeatCnt, p->repeatIntvl, p->preambleExtend);
	send_byte_to_ble(RESPONSE_CODE_SUCCESS);
}
	
static void cmd_send_and_listen(const uint8_t *pBuf, uint16_t len) 
{
	uint16_t sendPktLen = 0;
	uint8_t encodePkt[SUBG_MAX_PKT_LEN] = {0};
	uint8_t encodePktLen = 0;
	
	uint8_t getPkt[SUBG_MAX_PKT_LEN] = {0};
	uint8_t getPktLen = 0;
	eSubgRxStatus_t result;

	stSubgRespPkt_t subgResp;
	uint8_t decodePkt[SUBG_MAX_PKT_LEN] = {0};
	uint16_t decodePktLen = 0;
	
	stCmdSendAndListen_t *p = (stCmdSendAndListen_t *)pBuf;
	
	Kit_ReverseTwoBytes((uint16_t *)&p->repeatIntvl);
	Kit_ReverseFourBytes((uint32_t *)&p->listenTimeout);
	Kit_ReverseTwoBytes((uint16_t *)&p->preambleExtend);
	
	sendPktLen = len - (p->sendPkt - (uint8_t *)p);
	
	KIT_LOG(TAG, "Len: %d, repeat cnt: %d, repeat intvl: %d.", len, p->repeatCnt, p->repeatIntvl);
	KIT_LOG(TAG, "Listen timeout: %d, retry cnt: %d, preamble extend: %d.", p->listenTimeout, p->retryCnt, p->preambleExtend);

	if ((sendPktLen > 0 && p->sendPkt[sendPktLen - 1] == 0) && (Subg_GetMode() != SUBG_MODE_OMNIPOD))
	{
		sendPktLen--;
		KIT_LOG(TAG, "Last byte is 0, len - 1.");
	}

	encodePktLen = encrypt_encode(p->sendPkt, encodePkt, sendPktLen);
	
	Kit_PrintBytes(TAG, "Send to subg:", (const uint8_t*)encodePkt, encodePktLen);

	Subg_SendPkt(encodePkt, encodePktLen, p->repeatCnt, p->repeatIntvl, p->preambleExtend);
	result = Subg_GetPkt(getPkt, &getPktLen, p->listenTimeout, usePktLen);

	while(result == SUBG_RX_TIMEOUT && p->retryCnt > 0)
	{
		KIT_LOG(TAG, "Retry send and listen!", result);
		Subg_SendPkt(encodePkt, encodePktLen, 0, p->repeatIntvl, p->preambleExtend);
		result = Subg_GetPkt(getPkt, &getPktLen, p->listenTimeout, usePktLen);
		p->retryCnt--;
	}	

	switch(result)
	{
		case SUBG_RX_OK:		
			subgResp.rssi = convert_rssi_to_cc111x(Subg_GetRssi());
			subgResp.pktCnt = (uint8_t)(Subg_GetRxPktCnt() & 0xff);
			Kit_PrintBytes(TAG, "Rx done:", (const uint8_t*)getPkt, getPktLen);
			decodePktLen = encrypt_decode(getPkt, decodePkt, getPktLen);
			memcpy(subgResp.pkt, decodePkt, decodePktLen);
			send_bytes_to_ble((const uint8_t *)&subgResp, 2 + decodePktLen);
			break;
			
		case SUBG_RX_TIMEOUT:
			KIT_LOG(TAG, "Resp: rx timeout!");
			send_byte_to_ble(RESPONSE_CODE_RX_TIMEOUT);
			break;

		case SUBG_RX_INT:
			KIT_LOG(TAG, "Resp: rx interrupted!");
			send_byte_to_ble(RESPONSE_CODE_CMD_INTERRUPTED);
			break;
			
		default:
			break;
	}
}
 
static void cmd_update_reg(const uint8_t *pBuf, uint16_t len) 
{
	uint8_t addr = pBuf[0];
	uint8_t value = pBuf[1];
	
	// AAPS sends 2 bytes, Loop sends 10
	if (len < 2) 
	{
		//KIT_LOG(TAG, "Upd reg: len = %d, error.", len);
		return;
	}
	
	switch(addr) 
	{			
		case 0x02:
			KIT_LOG(TAG, "Upd reg: set pkt_len: 0x%02x", value);
			usePktLen = 1;
			Subg_SetPktLen(value);
			break;

		case 0x09:
		case 0x0A:
		case 0x0B:
			KIT_LOG(TAG, "Upd reg: addr 0x%02X, value 0x%02X.", addr, value);
			subgFreqReg[addr - 0x09] = value;
			check_and_set_freq();
			break;
			
		case 0x0C:
			if((value == 0x59) && (Subg_GetMode() != SUBG_MODE_MINIMED_WWL))
			{
				Subg_SetMode(SUBG_MODE_MINIMED_WWL);
				Subg_CfgRf();
			}
			break;
			
		default:
			KIT_LOG(TAG, "Upd reg: addr 0x%02X, ignored!", addr);
			break;
	}
	send_byte_to_ble(RESPONSE_CODE_SUCCESS);
}

static void cmd_led_mode(void) 
{
	send_byte_to_ble(RESPONSE_CODE_SUCCESS);
}

static void cmd_read_reg(const uint8_t *pBuf, uint16_t len) 
{
	uint8_t addr = pBuf[0];
	uint8_t value = 0;
	
	// APS sends 2 bytes, Loop sends 10
	if (len < 1) 
	{
		//KIT_LOG(TAG, "Upd reg: len = %d, error.", len);
		return;
	}

	switch(addr) 
	{		  
		case 0x09:
		case 0x0A:
		case 0x0B:
			value = subgFreqReg[addr - 0x09];
		  break;
		  		  
		default:
		  value = 0x5A;
	}
	send_bytes_to_ble((const uint8_t *)(&value), 1);
}


void cmd_set_preamble(const uint8_t *pBuf, uint16_t len) 
{
	Subg_SetPreamble((pBuf[0]<<8) | pBuf[1]);
	send_byte_to_ble(RESPONSE_CODE_SUCCESS);
}

static void cmd_reset_radio_config(void) 
{
	Subg_CfgRf();
	encrypt_set(ENCRYPT_NONE);
	Subg_SetPreamble(0);
	send_byte_to_ble(RESPONSE_CODE_SUCCESS);
}

static void cmd_get_statistics(void)
{
	stCmdGetStatisticsRespPkt_t statistics;

	statistics.updTime = apsCmdLoopCnt * APS_CMD_LOOP_TIME_MS;
	Kit_ReverseFourBytes((uint32_t *)&statistics.updTime);
	statistics.rxOverflowCnt = 0;
	statistics.rxFifoOverflowCnt = 0;
	statistics.pktRxCnt = Subg_GetRxPktCnt();
	Kit_ReverseTwoBytes((uint16_t *)&statistics.pktRxCnt);
	statistics.pktTxCnt = Subg_GetTxPktCnt();
	Kit_ReverseTwoBytes((uint16_t *)&statistics.pktTxCnt);
	statistics.crcFailCnt = 0;
	statistics.spiSyncFailCnt = 0;
	statistics.placeholder0 = 0; // Placeholder
	statistics.placeholder1 = 0; // Placeholder
	send_bytes_to_ble((const uint8_t *)(&statistics), sizeof(statistics));
}

static void aps_cmd_loop(void *pContext) 
{
	stApsReqPkt_t req;
	
	apsCmdLoopCnt++;
	if(!Kit_FifoStructOut(&apsCmdQueue, (void *)&req, 1)) 
	{
		return;
	}
		
	Subg_ClrIntFlg();
	switch (req.cmd) 
	{
		case CMD_GET_STATE:
			KIT_LOG(TAG, "CMD_GET_STATE.");
			cmd_get_state();
			break;
			
		case CMD_GET_VER:
			KIT_LOG(TAG, "CMD_GET_VER.");
			cmd_get_version();
			break;
			
		case CMD_GET_PKT:
			//KIT_LOG(TAG, "CMD_GET_PKT.");
			cmd_get_pkt(req.pkt);
			break;
			
		case CMD_SEND_PKT:
			//KIT_LOG(TAG, "CMD_SEND_PKT.");
			cmd_send_pkt(req.pkt, req.pktLen);
			break;
			
		case CMD_SEND_AND_LISTEN:
			KIT_LOG(TAG, "CMD_SEND_AND_LISTEN.");
			cmd_send_and_listen(req.pkt, req.pktLen);
			break;
			
		case CMD_UPDATE_REG:
			//KIT_LOG(TAG, "CMD_UPDATE_REG.");
			cmd_update_reg(req.pkt, req.pktLen);
			break;
			
		case CMD_LED:
			//KIT_LOG(TAG, "CMD_LED.");
			cmd_led_mode();
			break;
			
		case CMD_READ_REG:
			KIT_LOG(TAG, "CMD_READ_REG.");
			cmd_read_reg(req.pkt, req.pktLen);
			break;
			
		case CMD_SET_MODE_REG:
			KIT_LOG(TAG, "CMD_SET_MODE_REG.");
			cmd_set_mode_reg();
			break;
			
		case CMD_SET_SW_ENCODING:
			KIT_LOG(TAG, "CMD_SET_SW_ENCODING.");
			cmd_set_sw_encoding(req.pkt, req.pktLen);
			break;
			
		case CMD_SET_PREAMBLE:
			KIT_LOG(TAG, "CMD_SET_PREAMBLE.");
			cmd_set_preamble(req.pkt, req.pktLen);
			break;
			
		case CMD_RESET_RADIO_CFG:
			KIT_LOG(TAG, "CMD_RESET_RADIO_CFG.");
			cmd_reset_radio_config();
			break;
			
		case CMD_GET_STATISTICS:
			KIT_LOG(TAG, "CMD_GET_STATISTICS.");
			cmd_get_statistics();
			break;

		default:
			KIT_LOG(TAG, "Unkown cmd 0x%02x.", req.cmd);
			break;
	}
}

void Aps_PutCmd(const uint8_t *pBuf, uint16_t len, int8_t rssi) 
{
	if (len == 0) 
	{
		//KIT_LOG(TAG, "Put cmd: len = 0, return!");
		return;
	}
	
	if (pBuf[0] != len - 1 || len == 1) 
	{
		//KIT_LOG(TAG, "Put cmd: len = %d, byte0 = 0x%02X, return!", len, pBuf[0]);
		return;
	}
	
	eCmdTypes_t cmd = (eCmdTypes_t)pBuf[1];
	if (cmd == CMD_UPDATE_REG) 
	{
		KIT_LOG(TAG, "Put CMD_UPDATE_REG, do it now!");
		cmd_update_reg(pBuf + 2, len - 2);
		return;
	}
	
	stApsReqPkt_t req = 
	{
		.cmd = cmd,
		.pktLen = len - 2,
		.rssi = rssi,
	};
	memcpy(req.pkt, pBuf + 2, req.pktLen);
	
	if(!Kit_FifoStructIn(&apsCmdQueue, (void*)&req, 1)) 
	{
		KIT_LOG(TAG, "Cannot queue cmd 0x%02x.", cmd);
		return;
	}
	
	Subg_SetIntFlg();	
}

void Aps_Init(void)
{
	Kit_FifoStructCreate(&apsCmdQueue, (void*)apsCmdBuf, sizeof(apsCmdBuf), sizeof(stApsReqPkt_t));
	app_timer_create(&apsCmdLoopTimer, APP_TIMER_MODE_REPEATED, aps_cmd_loop);
	KIT_LOG(TAG, "Init OK!");
}

void Aps_StartLoop(void)
{
	if(!apsLoopStart)
	{
		apsLoopStart = true;
		app_timer_start(apsCmdLoopTimer, APP_TIMER_TICKS(APS_CMD_LOOP_TIME_MS), NULL);
		KIT_LOG(TAG, "Loop start!");
	}
}

void Aps_StopLoop(void)
{
	if(apsLoopStart)
	{
		apsLoopStart = false;
		app_timer_stop(apsCmdLoopTimer);
		KIT_LOG(TAG, "Loop stop!");
	}
}


