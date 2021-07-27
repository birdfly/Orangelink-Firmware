#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__
#include <stdint.h>

#define CFG_REQ_HEADER			0xdd

#ifdef __cplusplus
extern "C" {
#endif

void Cfg_PutReq(const uint8_t *pBuf, uint16_t len);
void Cfg_StartLoop(void);
void Cfg_StopLoop(void);
void Cfg_StorageLoad(void);
void Cfg_SetAdvName(const uint8_t *data, uint16_t len);
uint8_t* Cfg_GetAdvName(void);
uint8_t Cfg_GetAdvNameLen(void);
uint8_t Cfg_GetMotionData0(void);
uint8_t Cfg_GetMotionData1(void);
void Cfg_Init(void);

#ifdef __cplusplus
}
#endif

#endif
