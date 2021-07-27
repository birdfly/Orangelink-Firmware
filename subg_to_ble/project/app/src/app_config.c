#include "app_timer.h"
#include "ble_ips.h"
#include "kit_fifo.h"
#include "kit_log.h"
#include "led.h"
#include "motor.h"
#if defined(BOARD_XH_5102)	
#include "buzzer.h"
#endif
#include "boards.h"
#include "ocp.h"
#include "app_ble.h"
#include "app_battery.h"
#include "app_config.h"

#define TAG "CFG"

#define CFG_STORE_ADDR			FLASH_PAGE_0_ADDR
#define CFG_INIT_FLG			0xAA55AA55
#define CFG_MOTION_DATA_SIZE	16 

#define CFG_REQ_PARA_MAX_LEN	2
#define CFG_REQ_QUEUE_SIZE		2// 2 = actually only one
#define CFG_REQ_LOOP_TIME_MS   	100

#define CFG_RESP_SUCCESS 		0xaa
#define CFG_RESP_SENSOR_FAIL 	0xbb
#define CFG_RESP_PARAM_ERROR 	0xcc
#define CFG_RESP_UNKNOWN_CMD 	0xdd

#define CFG_RESP_HEADER_TYPE_ERR_LEN   	3

#define CFG_UPDATE_TIMEOUT     	20

APP_TIMER_DEF(m_cfg_update_timer_id);
APP_TIMER_DEF(cfgReqLoopTimer);

typedef enum
{
    CFG_REQ_MOTION_DATA_GET = 0x01,
	CFG_REQ_MOTION_DATA_SET,
	CFG_REQ_BATT_VOLT_GET,
	CFG_REQ_CALLING,
	CFG_REQ_NONE = 0xff
}eCfgReqType_t;

typedef struct
{
	uint8_t advName[BLE_IPS_MAX_CUS_NAME_CHAR_LEN];
	uint8_t advNameLen;
	uint32_t advNameInitFlg;
}stBleCfg_t;

typedef struct
{
	uint8_t data[CFG_MOTION_DATA_SIZE];
	uint32_t initFlg;
}stMotionCfg_t;

typedef struct
{
	stBleCfg_t ble;
	stMotionCfg_t motion;
}stCfgStorage_t;

typedef struct __attribute__((packed)) 
{
	uint8_t	data[CFG_MOTION_DATA_SIZE];
}stRespMotion_t;

typedef struct __attribute__((packed)) 
{
	uint8_t	voltHigh;
	uint8_t	voltLow;
}stRespBatt;

typedef struct __attribute__((packed)) 
{
	uint8_t header;
	uint8_t type;
	uint8_t errCode;
    union 
    {
		stRespMotion_t motion;
		stRespBatt batt;
    }__attribute__((packed)) para;
}stCfgRespPkt_t;

typedef struct
{
	eCfgReqType_t type;
	uint8_t paraLen;
	uint8_t para[CFG_REQ_PARA_MAX_LEN];
}stCfgReqPkt_t;

static stKitFifoStruct_t cfgReqQueue;
static stCfgReqPkt_t cfgReqBuf[CFG_REQ_QUEUE_SIZE];
static stCfgRespPkt_t cfgRespPkt = 
{
	.header = CFG_REQ_HEADER,
	.type = CFG_REQ_NONE,
	.errCode = CFG_RESP_SENSOR_FAIL,
	.para = {0}
};

static stCfgStorage_t config;
static bool cfgLoopStart = false;

static void cfg_update_handle(void * p_context)
{
	Flash_Write((uint32_t)CFG_STORE_ADDR, (void const *)(&config), sizeof(stCfgStorage_t));
	KIT_LOG(TAG, "Update success!");
}

static void motion_data_get(void) 
{	
	cfgRespPkt.type = CFG_REQ_MOTION_DATA_GET;	
	cfgRespPkt.errCode = CFG_RESP_SUCCESS;
	memcpy(cfgRespPkt.para.motion.data, config.motion.data, CFG_MOTION_DATA_SIZE);
	Ble_NusSendData((uint8_t *)&cfgRespPkt, sizeof(stRespMotion_t) + CFG_RESP_HEADER_TYPE_ERR_LEN);
}

static void motion_data_set(const uint8_t *pBuf, uint8_t len) 
{	 
	cfgRespPkt.type = CFG_REQ_MOTION_DATA_SET;
	if(len != 2)
	{
		cfgRespPkt.errCode = CFG_RESP_PARAM_ERROR;
	}
	else
	{
		switch(pBuf[0])
		{
			case 0x00:
				config.motion.data[0] = pBuf[1];
				if(pBuf[1] == 1)
				{
					Led_Ctrl(LED_ACT_YELLOW_TWINKLE, 0);
				}
				else
				{
					Led_Ctrl(LED_ACT_NONE, 0);
				}
				break;
				
			case 0x01:
				config.motion.data[1] = pBuf[1];
				if(pBuf[1] == 1)
				{
					Motor_Ctrl(MTR_ACT_SHORT_VIBRATE, 1);
				}
				else
				{
					Motor_Ctrl(MTR_ACT_NONE, 0);
				}
				break;
				
			default:
				break;
		}
		
		app_timer_start(m_cfg_update_timer_id, APP_TIMER_TICKS(CFG_UPDATE_TIMEOUT), NULL);
		cfgRespPkt.errCode = CFG_RESP_SUCCESS;
	}
	Ble_NusSendData((uint8_t *)&cfgRespPkt, CFG_RESP_HEADER_TYPE_ERR_LEN);
}

static void batt_volt_get(void) 
{	
	cfgRespPkt.type = CFG_REQ_BATT_VOLT_GET;	
	cfgRespPkt.errCode = CFG_RESP_SUCCESS;

	cfgRespPkt.para.batt.voltHigh = (uint8_t)(Batt_GetVoltage() >> 8);
	cfgRespPkt.para.batt.voltLow = (uint8_t)(Batt_GetVoltage() & 0xff);
	Ble_NusSendData((uint8_t *)&cfgRespPkt, sizeof(stRespBatt) + CFG_RESP_HEADER_TYPE_ERR_LEN);
}

#if defined(BOARD_XH_5102)	
static void calling(void) 
{	
	cfgRespPkt.type = CFG_REQ_CALLING;	
	cfgRespPkt.errCode = CFG_RESP_SUCCESS;

	Buzzer_Ctrl(BUZ_ACT_SERIAL_RING, 1);
	Ble_NusSendData((uint8_t *)&cfgRespPkt, CFG_RESP_HEADER_TYPE_ERR_LEN);
}
#endif

static void req_type_err(uint8_t type) 
{		
	cfgRespPkt.type = type;
	cfgRespPkt.errCode = CFG_RESP_UNKNOWN_CMD;
	Ble_NusSendData((uint8_t *)&cfgRespPkt, CFG_RESP_HEADER_TYPE_ERR_LEN);
}

static void cfg_req_loop(void *pContext) 
{
	stCfgReqPkt_t req;
	
	if(!Kit_FifoStructOut(&cfgReqQueue, (void *)&req, 1)) 
	{
		return;
	}
		
	switch(req.type) 
	{			
		case CFG_REQ_MOTION_DATA_GET:
			KIT_LOG(TAG, "CFG_REQ_MOTION_DATA_GET.");
			motion_data_get();
			break;
			
		case CFG_REQ_MOTION_DATA_SET:
			KIT_LOG(TAG, "CFG_REQ_MOTION_DATA_SET.");
			motion_data_set(req.para, req.paraLen);
			break;
			
		case CFG_REQ_BATT_VOLT_GET:
			KIT_LOG(TAG, "CFG_REQ_BATT_VOLT_GET.");
			batt_volt_get();
			break;
			
		#if defined(BOARD_XH_5102)	
		case CFG_REQ_CALLING:
			KIT_LOG(TAG, "CFG_REQ_CALLING.");
			calling();
			break;
		#endif
		
		default:
			req_type_err((uint8_t)req.type);
			KIT_LOG(TAG, "Unkown cmd 0x%02x.", req.type);
			break;
	}
}


void Cfg_PutReq(const uint8_t *pBuf, uint16_t len) 
{
	if (len == 0 || !cfgLoopStart) 
	{
		KIT_LOG(TAG, "Put cmd: len = 0 or loop not start, return!");
		return;
	}
	
	eCfgReqType_t type = (eCfgReqType_t)pBuf[0];	
	
	stCfgReqPkt_t req = 
	{
		.type = type,
		.paraLen = len - 1,
	};
	
	memcpy(req.para, pBuf + 1, req.paraLen);
	
	if (!Kit_FifoStructIn(&cfgReqQueue, (void*)&req, 1)) 
	{
		KIT_LOG(TAG, "Cannot queue cmd 0x%02x.", type);
		return;
	}
}

void Cfg_StartLoop(void)
{
	if(!cfgLoopStart)
	{
		cfgLoopStart = true;
		app_timer_start(cfgReqLoopTimer, APP_TIMER_TICKS(CFG_REQ_LOOP_TIME_MS), NULL);
		KIT_LOG(TAG, "Loop start!");
	}
}

void Cfg_StopLoop(void)
{
	if(cfgLoopStart)
	{
		cfgLoopStart = false;
		app_timer_stop(cfgReqLoopTimer);
		KIT_LOG(TAG, "Loop stop!");
	}
}

void Cfg_StorageLoad(void)
{
	bool flag = false;
	
	//fstorage calling must after stack init.
	Flash_Read(&config, CFG_STORE_ADDR, sizeof(config));
	if(config.ble.advNameInitFlg != CFG_INIT_FLG)
	{
		config.ble.advNameLen = strlen(BLE_DEFAULT_NAME);
		memcpy(config.ble.advName, BLE_DEFAULT_NAME, config.ble.advNameLen);
		config.ble.advNameInitFlg = CFG_INIT_FLG;
		flag = true;
	}
	
	if(config.motion.initFlg != CFG_INIT_FLG)
	{
		config.motion.data[0] = 1;
		config.motion.data[1] = 1;
		config.motion.initFlg = CFG_INIT_FLG;
		flag = true;
	}

	if(flag)
	{
		Flash_Write((uint32_t)CFG_STORE_ADDR, (void const *)(&config), sizeof(stCfgStorage_t));
	}
}

void Cfg_SetAdvName(const uint8_t *data, uint16_t len)
{
	memcpy(config.ble.advName, data, len);
	//config.ble.advNameInitFlg = CFG_INIT_FLG;
	config.ble.advNameLen = len;
	app_timer_start(m_cfg_update_timer_id, CFG_UPDATE_TIMEOUT, NULL);
}

uint8_t* Cfg_GetAdvName(void)
{
	return config.ble.advName;
}

uint8_t Cfg_GetAdvNameLen(void)
{
	return config.ble.advNameLen;
}

uint8_t Cfg_GetMotionData0(void)
{
	return config.motion.data[0];
}

uint8_t Cfg_GetMotionData1(void)
{
	return config.motion.data[1];
}

void Cfg_Init(void)
{
    ret_code_t err_code;
	
	Kit_FifoStructCreate(&cfgReqQueue, (void*)cfgReqBuf, sizeof(cfgReqBuf), sizeof(stCfgReqPkt_t));
	err_code = app_timer_create(&cfgReqLoopTimer, APP_TIMER_MODE_REPEATED, cfg_req_loop);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&m_cfg_update_timer_id, APP_TIMER_MODE_SINGLE_SHOT, cfg_update_handle);
    APP_ERROR_CHECK(err_code);
	KIT_LOG(TAG, "Init OK!");
}


