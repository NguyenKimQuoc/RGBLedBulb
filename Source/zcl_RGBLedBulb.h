/**************************************************************************************************
  Filename:       zcl_genericapp.h
  Revised:        $Date: 2014-06-19 08:38:22 -0700 (Thu, 19 Jun 2014) $
  Revision:       $Revision: 39101 $

  Description:    This file contains the ZigBee Cluster Library Home
                  Automation Sample Application.


  Copyright 2006-2014 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

#ifndef ZCL_RGBLedBulb_H
#define ZCL_RGBLedBulb_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "zcl.h"
#include "zcl_lighting.h"
#include "zcl_general.h"  
#ifndef ZCL_ON_OFF
#error ZCL_ON_OFF should be globally defined to process on/off commands.
#endif
#ifndef ZCL_IDENTIFY
#error ZCL_IDENTIFY should be globally defined to process identify commands.
#endif
#ifndef ZCL_GROUPS
#error ZCL_GROUPS should be globally defined to process group commands.
#endif
#ifndef ZCL_SCENES
#error ZCL_SCENES should be globally defined to process scene commands.
#endif
#ifndef ZCL_LIGHT_LINK_ENHANCE
#error ZCL_LIGHT_LINK_ENHANCE should be globally defined to process light link commands.
#endif

#include "hw_light_ctrl.h"

#ifdef ZCL_LEVEL_CTRL
#include "zcl_level_ctrl.h"
#endif //ZCL_LEVEL_CTRL

#ifdef ZCL_COLOR_CTRL
#include "zcl_color_ctrl.h"
  #ifndef ZCL_LEVEL_CTRL
    #error ZCL_LEVEL_CTRL should be globally defined in color lighting devices.
  #endif
#endif //ZCL_LEVEL_CTRL

// Added to include ZLL Target functionality
#if defined ( BDB_TL_INITIATOR ) || defined ( BDB_TL_TARGET )
  #include "zcl_general.h"
  #include "bdb_tlCommissioning.h"
#endif

/*********************************************************************
 * CONSTANTS
 */
#define RGBLedBulb_ENDPOINT            1
// Added to include ZLL Target functionality
#define RGBLedBulb_NUM_GRPS            2

  #ifdef ZCL_COLOR_CTRL
  #ifdef ZLL_HW_LED_LAMP
    #define RGBLedBulb_NUM_ATTRIBUTES      46
  #else
    #define RGBLedBulb_NUM_ATTRIBUTES      37
  #endif
  #define RGBLedBulb_SCENE_EXT_FIELD_SIZE  22
#else
  #ifdef ZCL_LEVEL_CTRL
    #define RGBLedBulb_NUM_ATTRIBUTES      22
    #define RGBLedBulb_SCENE_EXT_FIELD_SIZE 8
  #else
    #define RGBLedBulb_NUM_ATTRIBUTES      20
   #define RGBLedBulb_SCENE_EXT_FIELD_SIZE  4
  #endif
#endif
#define LIGHT_OFF                            0x00
#define LIGHT_ON                             0x01
   
// Application Events
#define RGBLedBulb_MAIN_SCREEN_EVT          0x0001
#define RGBLedBulb_EVT_LONG                 0x0002
#define RGBLedBulb_END_DEVICE_REJOIN_EVT    0x0004  
  
/* RGBLedBulb_TODO: define app events here */
  
#define RGBLedBulb_REPORTING_EVT                0x0008
#define RGBLedBulb_RST_COUNTING_EVT             0x0010
//#define COLOR_PROCESS_EVT                       0x0020
#define RGBLedBulb_IDENTIFY_TIMEOUT_EVT     0x0020
#define RGBLedBulb_EFFECT_PROCESS_EVT       0x0040
#define RGBLedBulb_ON_TIMED_OFF_TIMER_EVT   0x0080
#define RGBLedBulb_LEVEL_PROCESS_EVT        0x0100
#define RGBLedBulb_COLOR_PROCESS_EVT        0x0200
#define RGBLedBulb_COLOR_LOOP_PROCESS_EVT   0x0400
#define RGBLedBulb_THERMAL_SAMPLE_EVT       0x0800
/*
#define RGBLedBulb_EVT_2                    0x0010
#define RGBLedBulb_EVT_3                    0x0020
*/

// NV IDs
#define NV_PW_SW_COUTING_ID     0x0401
  
// Application Display Modes
#define GENERIC_MAINMODE      0x00
#define GENERIC_HELPMODE      0x01
  
#define RGBLedBulb_END_DEVICE_REJOIN_DELAY 10000

/*********************************************************************
 * MACROS
 */
#define SCENE_VALID() zclRGBLedBulb_SceneValid = 1;
#define SCENE_INVALID() zclRGBLedBulb_SceneValid = 0;
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
extern uint8  zclRGBLedBulb_OnOff;
extern uint8  zclRGBLedBulb_GlobalSceneCtrl;
extern uint16 zclRGBLedBulb_OnTime;
extern uint16 zclRGBLedBulb_OffWaitTime;
extern zclGeneral_Scene_t  zclRGBLedBulb_GlobalScene;

extern uint16 zclRGBLedBulb_IdentifyTime;

// Scene Cluster (server) -----------------------------------------------------
extern uint8 zclRGBLedBulb_CurrentScene;
extern uint16 zclRGBLedBulb_CurrentGroup;
extern uint8 zclRGBLedBulb_SceneValid;

// Added to include ZLL Target functionality
#if defined ( BDB_TL_INITIATOR ) || defined ( BDB_TL_TARGET )
  extern bdbTLDeviceInfo_t tlRGBLedBulb_DeviceInfo;
#endif

extern SimpleDescriptionFormat_t zclRGBLedBulb_SimpleDesc;

extern CONST zclCommandRec_t zclRGBLedBulb_Cmds[];

extern CONST uint8 zclCmdsArraySize;

// attribute list
extern CONST zclAttrRec_t zclRGBLedBulb_Attrs[];
extern CONST uint8 zclRGBLedBulb_NumAttributes;

// Identify attributes
extern uint16 zclRGBLedBulb_IdentifyTime;
extern uint8  zclRGBLedBulb_IdentifyCommissionState;

// RGBLedBulb_TODO: Declare application specific attributes here
ZStatus_t zclRGBLedBulb_MoveToColorCB( zclCCMoveToColor_t *pCmd );

/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void zclRGBLedBulb_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 zclRGBLedBulb_event_loop( byte task_id, UINT16 events );

/*
 *  Reset all writable attributes to their default values.
 */
extern void zclRGBLedBulb_ResetAttributesToDefaultValues(void);


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZCL_RGBLedBulb_H */
