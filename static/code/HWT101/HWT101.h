#ifndef __HWT101_h
#define __HWT101_h

#include <string.h>
#include <stdint.h>
#include "SysConfig.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define HWT101_UART &huart2

extern bool HWT_Ready_Flag;
extern float HWT_Yaw_Angle;
extern float HWT_Y_Speed;
extern float HWT_Z_Speed;

void HWT101_Init(void);
bool HWT101_Read(void);
void HWT101_Handler(uint16_t size);

#ifdef __cplusplus
}
#endif


#endif
