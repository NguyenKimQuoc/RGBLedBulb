
#ifndef HW_COLOR_CTRL_H
#define HW_COLOR_CTRL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include "zcl_lighting.h"
#include "pwm.h"
#include "uart.h"
#include "zcl_RGBLedBulb.h"
#define LEVEL_MAX                       0xFE
#define GAMMA_VALUE                     2           
#define PWM_FULL_DUTY_CYCLE             255
#define COLOR_XY_MIN                    0x0
#define COLOR_XY_MAX                    LIGHTING_COLOR_CURRENT_X_MAX
#define COLOR_HUE_MIN                   0x0
#define COLOR_HUE_MAX                   LIGHTING_COLOR_HUE_MAX
#define COLOR_SAT_MIN                0x0
#define COLOR_SAT_MAX                LIGHTING_COLOR_SAT_MAX
#define WHITE_POINT_X 0x5000
#define WHITE_POINT_Y 0x5555


extern uint8 zclLevel_CurrentLevel;
extern uint16 zclRGBLedBulb_CurrentX;
extern uint16 zclRGBLedBulb_CurrentY;
extern uint16 zclRGBLedBulb_EnhancedCurrentHue;
extern uint8  zclRGBLedBulb_CurrentHue;
extern uint8  zclRGBLedBulb_CurrentSaturation;
extern uint8  zclRGBLedBulb_ColorMode;
extern uint8  zclRGBLedBulb_EnhancedColorMode;
extern uint16 zclRGBLedBulb_ColorRemainingTime;
extern uint8 zclRGBLedBulb_ColorLoopActive;
extern uint8 zclRGBLedBulb_ColorLoopDirection;
extern uint16 zclRGBLedBulb_ColorLoopTime;
extern uint16 zclRGBLedBulb_ColorLoopStartEnhancedHue;
extern uint16 zclRGBLedBulb_ColorLoopStoredEnhancedHue;

void zclRGBLedBulb_init( byte taskID);
void zclRGBLedBulb_process( uint16 *events );

ZStatus_t zclRGBLedBulb_MoveToColorCB( zclCCMoveToColor_t *pCmd );
void hwLight_UpdateColorMode(uint8 NewColorMode);
void hwLight_ApplyUpdate( uint8 *pCurrentVal, uint16 *pCurrentVal_256,
                          int32 *pStepVal_256, uint16 *pRemainingTime,
                          uint8 minLevel, uint8 maxLevel, bool wrap );
void hwLight_ApplyUpdate16b( uint16 *pCurrentVal, uint32 *pCurrentVal_256,
                             int32 *pStepVal_256, uint16 *pRemainingTime,
                             uint16 minLevel, uint16 maxLevel, bool wrap );
void hwLight_Refresh( uint8 refreshState );
void hwLight_UpdateColor(void);


#ifdef __cplusplus
}
#endif

#endif /* ZCL_COLOR_CNTRL_H */