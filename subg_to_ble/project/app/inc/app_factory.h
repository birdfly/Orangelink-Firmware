#ifndef __APP_FACTORY_H__
#define __APP_FACTORY_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FCT_REQ_SATRT_TEST			0xaa
#define FCT_REQ_HEADER 				0xbb
#define FCT_REQ_STOP_TEST			0xcc

void Fct_Init(void);
void Fct_StartLoop(void);
void Fct_StopLoop(void);
void Fct_PutReq(const uint8_t *pBuf, uint16_t len);
bool Fct_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif
