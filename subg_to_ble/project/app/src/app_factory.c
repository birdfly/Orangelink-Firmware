#include <stdint.h>
#include <stdlib.h> 

#include "app_timer.h"
#include "app_subg.h"
#include "kit_fifo.h"
#include "kit_log.h"
#include "app_ble.h"
#include "app_battery.h"
#include "ocp.h"
#include "boards.h"
#include "led.h"
#include "motor.h"
#include "app_battery.h"
#include "app_indication.h"
#include "app_factory.h"

#define TAG "FAC"

#define FCT_REQ_PARA_MAX_LEN	20
#define FCT_REQ_QUEUE_SIZE		2// 2 = actually only one
#define FCT_REQ_LOOP_TIME_MS   	50

#define FCT_RESP_SUCCESS 		0xaa
#define FCT_RESP_SENSOR_FAIL 	0xbb
#define FCT_RESP_PARAM_ERROR 	0xcc
#define FCT_RESP_UNKNOWN_CMD 	0xdd

#define FCT_RESP_HEADER_TYPE_ERR_LEN   	3

#define FCT_TEST_FREQ_433   	433923000
#define FCT_TEST_FREQ_868   	868388000
#define FCT_TEST_FREQ_916   	916548000

#define FCT_SN_INIT_FLG			0xAA55AA55
#define FCT_SN_STORE_ADDR		FLASH_PAGE_1_ADDR
#define FCT_SN_CODE_SIZE		4 

APP_TIMER_DEF(fctReqLoopTimer);

typedef enum
{
    FCT_REQ_YELLOW_LED_ON = 0x01,
    FCT_REQ_RED_LED_ON,
    FCT_REQ_LED_OFF, 
    FCT_REQ_MOTOR_VIBRATE,
    FCT_REQ_MOTOR_STOP, 
	FCT_REQ_SUBG_TEST,
	FCT_REQ_BATT_LEVEL_GET,
	FCT_REQ_SN_BURN,
	FCT_REQ_VERSION_GET,
	FCT_REQ_SN_GET,
	FCT_REQ_NONE = 0xff
}eFctReqType_t;

typedef struct __attribute__((packed)) 
{
	uint8_t	level;
}stDevBatt_t;

typedef struct __attribute__((packed)) 
{
	uint8_t	code[FCT_SN_CODE_SIZE];
}stDevSn_t;

typedef struct __attribute__((packed)) 
{
	uint8_t	onmipod;
	uint8_t	minimedwwl;
	uint8_t	minimednas;
}stSubgRssi_t;

typedef struct __attribute__((packed)) 
{
    uint8_t fwMajor;
    uint8_t fwMinor;
    uint8_t hwMajor;
    uint8_t hwMinor;
}stDevVersion_t;

typedef struct
{
	eFctReqType_t type;
	uint8_t paraLen;
	uint8_t para[FCT_REQ_PARA_MAX_LEN];
}stFctReqPkt_t;

typedef struct __attribute__((packed)) 
{
	uint8_t header;
	uint8_t type;
	uint8_t errCode;
    union 
    {
		stSubgRssi_t rssi;
		stDevVersion_t version;
		stDevBatt_t batt;
		stDevSn_t sn;
    }__attribute__((packed)) para;
}stFctRespPkt_t;

typedef struct
{
	uint8_t  code[FCT_SN_CODE_SIZE];
	uint32_t initFlg;
}stSnStorage_t;

static bool fctLoopStart = false;
static stKitFifoStruct_t fctReqQueue;
static stFctReqPkt_t fctReqBuf[FCT_REQ_QUEUE_SIZE];
static stFctRespPkt_t fctRespPkt = 
{
	.header = FCT_REQ_HEADER,
	.type = FCT_REQ_NONE,
	.errCode = FCT_RESP_SENSOR_FAIL,
	.para = {0}
};

static void yellow_led_on(void) 
{	 
	Led_Ctrl(LED_ACT_YELLOW_LIGHT, 0);
	fctRespPkt.type = FCT_REQ_YELLOW_LED_ON;
	fctRespPkt.errCode = FCT_RESP_SUCCESS;
	Ble_NusSendData((uint8_t *)&fctRespPkt, FCT_RESP_HEADER_TYPE_ERR_LEN);
}

static void red_led_on(void) 
{	 
	Led_Ctrl(LED_ACT_RED_LIGHT, 0);
	fctRespPkt.type = FCT_REQ_RED_LED_ON;
	fctRespPkt.errCode = FCT_RESP_SUCCESS;
	Ble_NusSendData((uint8_t *)&fctRespPkt, FCT_RESP_HEADER_TYPE_ERR_LEN);
}

static void all_led_off(void) 
{	 
	Led_Ctrl(LED_ACT_NONE, 0);
	fctRespPkt.type = FCT_REQ_LED_OFF;
	fctRespPkt.errCode = FCT_RESP_SUCCESS;
	Ble_NusSendData((uint8_t *)&fctRespPkt, FCT_RESP_HEADER_TYPE_ERR_LEN);
}

static void motor_start_vibrate(void) 
{	 
	Motor_Ctrl(MTR_ACT_LONG_VIBRATE, 0);
	fctRespPkt.type = FCT_REQ_MOTOR_VIBRATE;
	fctRespPkt.errCode = FCT_RESP_SUCCESS;
	Ble_NusSendData((uint8_t *)&fctRespPkt, FCT_RESP_HEADER_TYPE_ERR_LEN);
}

static void motor_stop_vibrate(void) 
{	 
	Motor_Ctrl(MTR_ACT_NONE, 0);
	fctRespPkt.type = FCT_REQ_MOTOR_STOP;
	fctRespPkt.errCode = FCT_RESP_SUCCESS;
	Ble_NusSendData((uint8_t *)&fctRespPkt, FCT_RESP_HEADER_TYPE_ERR_LEN);
}

static void subg_send_and_listen(void) 
{
	eSubgRxStatus_t result;
	uint8_t retryCnt;
	int successCnt;
	int rssi = 0;
	uint8_t rcvPkt[6] = {0};
	uint8_t rcvPktLen = 0;
	uint8_t sendPkt[] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};
	
	//KIT_LOG(TAG, "------------Omnipod test start------------");
	Subg_SetMode(SUBG_MODE_OMNIPOD);
	Subg_SetFreq(FCT_TEST_FREQ_433);
	successCnt = 0;
	rcvPktLen = 0;
	rssi = 0;
	fctRespPkt.para.rssi.onmipod = 0;
	for(retryCnt = 0; retryCnt < 3; retryCnt++)
	{
		Subg_SendPkt(sendPkt, sizeof(sendPkt), 0, 0, 30);
		result = Subg_GetPkt(rcvPkt, &rcvPktLen, 200, 0);
		if(result == SUBG_RX_OK)
		{
			if(rcvPktLen == 6 && rcvPkt[0] == 0xaa)
			{
				successCnt++;
				rssi += Subg_GetRssi();
				Kit_PrintBytes(TAG, "Rx done:", (const uint8_t*)rcvPkt, rcvPktLen);
			}
		}
		else if(result == SUBG_RX_INT)
		{
			return;
		}
	}
	fctRespPkt.para.rssi.onmipod = abs(rssi / successCnt);

	//KIT_LOG(TAG, "------------Minimed_nas test start------------");
	Subg_SetMode(SUBG_MODE_MINIMED_NAS);
	Subg_SetFreq(FCT_TEST_FREQ_916);
	successCnt = 0;
	rcvPktLen = 0;
	rssi = 0;
	fctRespPkt.para.rssi.minimednas = 0;
	for(retryCnt = 0; retryCnt < 3; retryCnt++)
	{
		Subg_SendPkt(sendPkt, sizeof(sendPkt), 0, 0, 0);
		result = Subg_GetPkt(rcvPkt, &rcvPktLen, 200, 0);
		if(result == SUBG_RX_OK)
		{
			if(rcvPktLen == 6 && rcvPkt[0] == 0xaa)
			{
				successCnt++;
				rssi += Subg_GetRssi();
				Kit_PrintBytes(TAG, "Rx done:", (const uint8_t*)rcvPkt, rcvPktLen);
			}
		}
		else if(result == SUBG_RX_INT)
		{
			return;
		}
	}
	fctRespPkt.para.rssi.minimednas = abs(rssi / successCnt);
	
	//KIT_LOG(TAG, "------------Minimed_wwl test start------------");
	Subg_SetMode(SUBG_MODE_MINIMED_WWL);
	Subg_SetFreq(FCT_TEST_FREQ_868);
	successCnt = 0;
	rcvPktLen = 0;
	rssi = 0;
	fctRespPkt.para.rssi.minimedwwl = 0;
	for(retryCnt = 0; retryCnt < 3; retryCnt++)
	{
		Subg_SendPkt(sendPkt, sizeof(sendPkt), 0, 0, 0);
		result = Subg_GetPkt(rcvPkt, &rcvPktLen, 200, 0);
		if(result == SUBG_RX_OK)
		{
			if(rcvPktLen == 6 && rcvPkt[0] == 0xaa)
			{
				successCnt++;
				rssi += Subg_GetRssi();
				Kit_PrintBytes(TAG, "Rx done:", (const uint8_t*)rcvPkt, rcvPktLen);
			}
		}
		else if(result == SUBG_RX_INT)
		{
			return;
		}
	}
	fctRespPkt.para.rssi.minimedwwl = abs(rssi / successCnt);
	
	fctRespPkt.type = FCT_REQ_SUBG_TEST;
	if((fctRespPkt.para.rssi.onmipod || fctRespPkt.para.rssi.minimednas || fctRespPkt.para.rssi.minimedwwl) == 0)
	{
		fctRespPkt.errCode = FCT_RESP_SENSOR_FAIL;
		Ble_NusSendData((uint8_t *)&fctRespPkt, FCT_RESP_HEADER_TYPE_ERR_LEN);
	}
	else
	{
		fctRespPkt.errCode = FCT_RESP_SUCCESS;
		Ble_NusSendData((uint8_t *)&fctRespPkt, sizeof(stSubgRssi_t) + FCT_RESP_HEADER_TYPE_ERR_LEN);
	}
}

static void batt_level_get(void) 
{	 
	fctRespPkt.type = FCT_REQ_BATT_LEVEL_GET;
	fctRespPkt.para.batt.level = Batt_GetLevel();
	if(fctRespPkt.para.batt.level > 0 && fctRespPkt.para.batt.level <= 100)
	{
		fctRespPkt.errCode = FCT_RESP_SUCCESS;
		Ble_NusSendData((uint8_t *)&fctRespPkt, sizeof(stDevBatt_t) + FCT_RESP_HEADER_TYPE_ERR_LEN);
	}
	else
	{
		fctRespPkt.errCode = FCT_RESP_SENSOR_FAIL;
		Ble_NusSendData((uint8_t *)&fctRespPkt, FCT_RESP_HEADER_TYPE_ERR_LEN);
	}
}

static void sn_burn(const uint8_t *pBuf, uint8_t len) 
{	 
	stSnStorage_t snStorage;

	Flash_Read(&snStorage, FCT_SN_STORE_ADDR, sizeof(snStorage));
	fctRespPkt.type = FCT_REQ_SN_BURN;
	
	if(snStorage.initFlg == FCT_SN_INIT_FLG)
	{
		fctRespPkt.errCode = FCT_RESP_SENSOR_FAIL;
	}
	else if(len != FCT_SN_CODE_SIZE)
	{
		fctRespPkt.errCode = FCT_RESP_PARAM_ERROR;
	}
	else
	{
		snStorage.initFlg = FCT_SN_INIT_FLG;
		memcpy(snStorage.code, pBuf, len);
		Flash_Write((uint32_t)FCT_SN_STORE_ADDR, &snStorage, sizeof(stSnStorage_t));
		fctRespPkt.errCode = FCT_RESP_SUCCESS;
	}
	Ble_NusSendData((uint8_t *)&fctRespPkt, FCT_RESP_HEADER_TYPE_ERR_LEN);
}

static void version_get(void) 
{	
	fctRespPkt.type = FCT_REQ_VERSION_GET;
	fctRespPkt.errCode = FCT_RESP_SUCCESS;
	fctRespPkt.para.version.fwMajor = FW_VERSION_MAJOR;
	fctRespPkt.para.version.fwMinor = FW_VERSION_MINOR;
	fctRespPkt.para.version.hwMajor = HW_VERSION_MAJOR;
	fctRespPkt.para.version.hwMinor = HW_VERSION_MINOR;
	Ble_NusSendData((uint8_t *)&fctRespPkt, sizeof(stDevVersion_t) + FCT_RESP_HEADER_TYPE_ERR_LEN);
}

static void sn_get(void) 
{	
	stSnStorage_t snStorage;
	
	Flash_Read(&snStorage, FCT_SN_STORE_ADDR, sizeof(snStorage));
	
	fctRespPkt.type = FCT_REQ_SN_GET;
	if(snStorage.initFlg != FCT_SN_INIT_FLG)
	{
		fctRespPkt.errCode = FCT_RESP_SENSOR_FAIL;
		Ble_NusSendData((uint8_t *)&fctRespPkt, FCT_RESP_HEADER_TYPE_ERR_LEN);
	}
	else
	{
		fctRespPkt.errCode = FCT_RESP_SUCCESS;
		memcpy(fctRespPkt.para.sn.code, snStorage.code, FCT_SN_CODE_SIZE);
		Ble_NusSendData((uint8_t *)&fctRespPkt, sizeof(stDevSn_t) + FCT_RESP_HEADER_TYPE_ERR_LEN);
	}
}

static void req_type_err(uint8_t type) 
{		
	fctRespPkt.type = type;
	fctRespPkt.errCode = FCT_RESP_UNKNOWN_CMD;
	Ble_NusSendData((uint8_t *)&fctRespPkt, FCT_RESP_HEADER_TYPE_ERR_LEN);
}

static void fct_req_loop(void *pContext) 
{
	stFctReqPkt_t req;
	
	if(!Kit_FifoStructOut(&fctReqQueue, (void *)&req, 1)) 
	{
		return;
	}
		
	Subg_ClrIntFlg();
	switch (req.type) 
	{
		case FCT_REQ_YELLOW_LED_ON:
			yellow_led_on();
			//KIT_LOG(TAG, "Cmd: FCT_REQ_YELLOW_LED_ON.");
			break;
			
		case FCT_REQ_RED_LED_ON:
			//KIT_LOG(TAG, "Cmd: FCT_REQ_RED_LED_ON.");
			red_led_on();
			break;
			
		case FCT_REQ_LED_OFF:
			all_led_off();
			//KIT_LOG(TAG, "Cmd: FCT_REQ_LED_OFF.");
			break;
			
		case FCT_REQ_MOTOR_VIBRATE:
			//KIT_LOG(TAG, "Cmd: FCT_REQ_MOTOR_VIBRATE.");
			motor_start_vibrate();
			break;
			
		case FCT_REQ_MOTOR_STOP:
			//KIT_LOG(TAG, "Cmd: FCT_REQ_MOTOR_STOP.");
			motor_stop_vibrate();
			break;
						
		case FCT_REQ_SUBG_TEST:
			//KIT_LOG(TAG, "Cmd: FCT_REQ_SUBG_TEST.");
			subg_send_and_listen();
			break;
						
		case FCT_REQ_BATT_LEVEL_GET:
			//KIT_LOG(TAG, "Cmd: FCT_REQ_BATT_LEVEL_GET.");
			batt_level_get();
			break;
			
		case FCT_REQ_SN_BURN:
			//KIT_LOG(TAG, "Cmd: FCT_REQ_SN_BURN.");
			sn_burn(req.para, req.paraLen);
			break;
			
		case FCT_REQ_VERSION_GET:
			//KIT_LOG(TAG, "Cmd: FCT_REQ_VERSION_GET.");
			version_get();
			break;
			
		case FCT_REQ_SN_GET:
			//KIT_LOG(TAG, "Cmd: FCT_REQ_SN_GET.");
			sn_get();
			break;
			
		default:
			req_type_err((uint8_t)req.type);
			//KIT_LOG(TAG, "Cmd: unkown cmd 0x%02x.", req.type);
			break;
	}
}

void Fct_Init(void)
{
	Kit_FifoStructCreate(&fctReqQueue, (void*)fctReqBuf, sizeof(fctReqBuf), sizeof(stFctReqPkt_t));
	app_timer_create(&fctReqLoopTimer, APP_TIMER_MODE_REPEATED, fct_req_loop);
	KIT_LOG(TAG, "Init OK!");
}

void Fct_PutReq(const uint8_t *pBuf, uint16_t len) 
{
	if (len == 0 || !fctLoopStart) 
	{
		//KIT_LOG(TAG, "Put cmd: len = 0 or loop not start, return!");
		return;
	}
	
	eFctReqType_t type = (eFctReqType_t)pBuf[0];	
	
	stFctReqPkt_t req = 
	{
		.type = type,
		.paraLen = len - 1,
	};
	
	memcpy(req.para, pBuf + 1, req.paraLen);
	
	if (!Kit_FifoStructIn(&fctReqQueue, (void*)&req, 1)) 
	{
		KIT_LOG(TAG, "Cannot queue cmd 0x%02x.", type);
		return;
	}
	
	Subg_SetIntFlg();	
}

void Fct_StartLoop(void)
{
	if(!fctLoopStart)
	{
		fctLoopStart = true;
		app_timer_start(fctReqLoopTimer, APP_TIMER_TICKS(FCT_REQ_LOOP_TIME_MS), NULL);
		Idc_SetType(INDICATE_FAC_TEST_START);
		KIT_LOG(TAG, "Loop start!");
	}
}

void Fct_StopLoop(void)
{
	if(fctLoopStart)
	{
		fctLoopStart = false;
		app_timer_stop(fctReqLoopTimer);
		Idc_SetType(INDICATE_FAC_TEST_STOP);
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
		KIT_LOG(TAG, "Loop stop!");
	}
}


