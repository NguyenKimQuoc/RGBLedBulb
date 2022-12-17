/**************************************************************************************************
  Filename:       zcl_RGBLedBulb_data.c
  Revised:        $Date: 2014-05-12 13:14:02 -0700 (Mon, 12 May 2014) $
  Revision:       $Revision: 38502 $


  Description:    Zigbee Cluster Library - sample device application.


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
  PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"

/* RGBLedBulb_TODO: Include any of the header files below to access specific cluster data
#include "zcl_poll_control.h"
#include "zcl_electrical_measurement.h"
#include "zcl_diagnostic.h"
#include "zcl_meter_identification.h"
#include "zcl_appliance_identification.h"
#include "zcl_appliance_events_alerts.h"
#include "zcl_power_profile.h"
#include "zcl_appliance_control.h"
#include "zcl_appliance_statistics.h"
#include "zcl_hvac.h"
*/

#include "zcl_RGBLedBulb.h"

/*********************************************************************
 * CONSTANTS
 */

#define RGBLedBulb_DEVICE_VERSION     1
#define RGBLedBulb_FLAGS              0

#define RGBLedBulb_HWVERSION          1
#define RGBLedBulb_ZCLVERSION         1

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// On/Off Cluster
uint8  zclRGBLedBulb_OnOff = LIGHT_OFF;
uint8  zclRGBLedBulb_GlobalSceneCtrl = TRUE;
uint16 zclRGBLedBulb_OnTime = 0x0000;
uint16 zclRGBLedBulb_OffWaitTime = 0x0000;
zclGeneral_Scene_t zclRGBLedBulb_GlobalScene =
{
  0,                                     // The group ID for which this scene applies
  0,                                     // Scene ID
  0,                                     // Time to take to transition to this scene
  0,                                     // Together with transTime, this allows transition time to be specified in 1/10s
  "GlobalScene",                         // Scene name
  ZCL_GEN_SCENE_EXT_LEN,                 // Length of extension fields
  0,                                     // Extension fields
};

// Level control Cluster (server) -----------------------------------------------------
#ifdef ZCL_LEVEL_CTRL
uint8 zclLevel_CurrentLevel = 0xFE;
uint16 zclLevel_LevelRemainingTime = 0;
#endif //ZCL_LEVEL_CTRL

// Scene Cluster (server) -----------------------------------------------------
uint8 zclRGBLedBulb_CurrentScene = 0x0;
uint16 zclRGBLedBulb_CurrentGroup = 0x0;
uint8 zclRGBLedBulb_SceneValid = 0x0;
const uint8 zclRGBLedBulb_SceneNameSupport = 0;

// Group Cluster (server) -----------------------------------------------------
const uint8 zclRGBLedBulb_GroupNameSupport = 0;

// Color control Cluster (server) -----------------------------------------------------
#ifdef ZCL_COLOR_CTRL
uint16 zclColor_CurrentX = 0x616b;
uint16 zclColor_CurrentY = 0x607d;
uint16 zclColor_EnhancedCurrentHue = 0;
uint8  zclColor_CurrentHue = 0;
uint8  zclColor_CurrentSaturation = 0x0;

uint8  zclColor_ColorMode = COLOR_MODE_CURRENT_X_Y;
uint8  zclColor_EnhancedColorMode = ENHANCED_COLOR_MODE_CURRENT_HUE_SATURATION;
uint16 zclColor_ColorRemainingTime = 0;
uint8  zclColor_ColorLoopActive = 0;
uint8  zclColor_ColorLoopDirection = 0;
uint16 zclColor_ColorLoopTime = 0x0019;
uint16 zclColor_ColorLoopStartEnhancedHue = 0x2300;
uint16 zclColor_ColorLoopStoredEnhancedHue = 0;
uint16 zclColor_ColorCapabilities = ( COLOR_CAPABILITIES_ATTR_BIT_HUE_SATURATION |
                                      COLOR_CAPABILITIES_ATTR_BIT_ENHANCED_HUE |
                                      COLOR_CAPABILITIES_ATTR_BIT_COLOR_LOOP |
                                      COLOR_CAPABILITIES_ATTR_BIT_X_Y_ATTRIBUTES );
#ifdef ZLL_HW_LED_LAMP
const uint8  zclColor_NumOfPrimaries = 3;
//RED: LR W5AP, 625nm
const uint16 zclColor_Primary1X = 0xB35B;
const uint16 zclColor_Primary1Y = 0x4C9F;
const uint8 zclColor_Primary1Intensity = 0x9F;
//GREEN: LT W5AP, 528nm
const uint16 zclColor_Primary2X = 0x2382;
const uint16 zclColor_Primary2Y = 0xD095;
const uint8 zclColor_Primary2Intensity = 0xF0;
//BLUE: LD W5AP, 455nm
const uint16 zclColor_Primary3X = 0x26A7;
const uint16 zclColor_Primary3Y = 0x05D2;
const uint8 zclColor_Primary3Intensity = 0xFE;
#else
const uint8  zclColor_NumOfPrimaries = 0;
#endif //ZLL_HW_LED_LAMP
#endif //ZCL_COLOR_CTRL

const uint16 zclRGBLedBulb_clusterRevision_all = 0x0001; 
// Basic Cluster
const uint8 zclRGBLedBulb_HWRevision = RGBLedBulb_HWVERSION;
const uint8 zclRGBLedBulb_ZCLVersion = RGBLedBulb_ZCLVERSION;
const uint8 zclRGBLedBulb_ManufacturerName[] = { 9, 'W', 'a', 'r', 'm', 'H', 'o', 'u', 's', 'e' };
const uint8 zclRGBLedBulb_ModelId[] = { 9, 'W', 'H', '_', 'L', 'E', 'D', 'R', 'G', 'B'};
const uint8 zclRGBLedBulb_DateCode[] = { 8, '2','0','2','1','1','2','1','9' };
const uint8 zclRGBLedBulb_PowerSource = POWER_SOURCE_MAINS_1_PHASE;

uint8 zclRGBLedBulb_LocationDescription[17] = { 16, ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
uint8 zclRGBLedBulb_PhysicalEnvironment = 0;
uint8 zclRGBLedBulb_DeviceEnable = DEVICE_ENABLED;

// Identify Cluster
uint16 zclRGBLedBulb_IdentifyTime;

/* RGBLedBulb_TODO: declare attribute variables here. If its value can change,
 * initialize it in zclRGBLedBulb_ResetAttributesToDefaultValues. If its
 * value will not change, initialize it here.
 */

#if ZCL_DISCOVER
CONST zclCommandRec_t zclRGBLedBulb_Cmds[] =
{
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    COMMAND_BASIC_RESET_FACT_DEFAULT,
    CMD_DIR_SERVER_RECEIVED
  },

};

CONST uint8 zclCmdsArraySize = ( sizeof(zclRGBLedBulb_Cmds) / sizeof(zclRGBLedBulb_Cmds[0]) );
#endif // ZCL_DISCOVER

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t zclRGBLedBulb_Attrs[] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zclRGBLedBulb_HWRevision  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclRGBLedBulb_ZCLVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclRGBLedBulb_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclRGBLedBulb_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclRGBLedBulb_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&zclRGBLedBulb_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zclRGBLedBulb_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclRGBLedBulb_PhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclRGBLedBulb_DeviceEnable
    }
  },

#ifdef ZCL_IDENTIFY
  // *** Identify Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclRGBLedBulb_IdentifyTime
    }
  },
#endif
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclRGBLedBulb_clusterRevision_all
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    {  // Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclRGBLedBulb_clusterRevision_all
    }
  },

// *** Scene Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_SCENES,
    { // Attribute record
      ATTRID_SCENES_COUNT,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ),
      NULL // Use application's callback to Read this attribute
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_SCENES,
    { // Attribute record
      ATTRID_SCENES_CURRENT_SCENE,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ),
      (void *)&zclRGBLedBulb_CurrentScene
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_SCENES,
    { // Attribute record
      ATTRID_SCENES_CURRENT_GROUP,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ),
      (void *)&zclRGBLedBulb_CurrentGroup
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_SCENES,
    { // Attribute record
      ATTRID_SCENES_SCENE_VALID,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ),
      (void *)&zclRGBLedBulb_SceneValid
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_SCENES,
    { // Attribute record
      ATTRID_SCENES_NAME_SUPPORT,
      ZCL_DATATYPE_BITMAP8,
      (ACCESS_CONTROL_READ),
      (void *)&zclRGBLedBulb_SceneNameSupport
    }
  },
  // *** Groups Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_GROUPS,
    { // Attribute record
      ATTRID_GROUP_NAME_SUPPORT,
      ZCL_DATATYPE_BITMAP8,
      (ACCESS_CONTROL_READ),
      (void *)&zclRGBLedBulb_GroupNameSupport
    }
  },

  // *** On/Off Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF,
      ZCL_DATATYPE_BOOLEAN,
      ACCESS_CONTROL_READ,
      (void *)&zclRGBLedBulb_OnOff
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF_GLOBAL_SCENE_CTRL,
      ZCL_DATATYPE_BOOLEAN,
      ACCESS_CONTROL_READ,
      (void *)&zclRGBLedBulb_GlobalSceneCtrl
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF_ON_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclRGBLedBulb_OnTime
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF_OFF_WAIT_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclRGBLedBulb_OffWaitTime
    }
  },
  // *** Level Control Cluster Attribute ***
#ifdef ZCL_LEVEL_CTRL
  {
    ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL,
    { // Attribute record
      ATTRID_LEVEL_CURRENT_LEVEL,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclLevel_CurrentLevel
    }
  },

  {
    ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL,
    { // Attribute record
      ATTRID_LEVEL_REMAINING_TIME,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclLevel_LevelRemainingTime
    }
  },
#endif //ZCL_LEVEL_CTRL
  // *** Color Control Cluster Attributes ***
#ifdef ZCL_COLOR_CTRL
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_CurrentSaturation
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_CurrentHue
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_REMAINING_TIME,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorRemainingTime
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_X,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ),
      (void *)&zclColor_CurrentX
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_Y,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ),
      (void *)&zclColor_CurrentY
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_MODE,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ),
      (void *)&zclColor_ColorMode
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_NUM_PRIMARIES,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ),
      (void *)&zclColor_NumOfPrimaries
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_ENHANCED_COLOR_MODE,
      ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ),
      (void *)&zclColor_EnhancedColorMode
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_ENHANCED_CURRENT_HUE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_EnhancedCurrentHue
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_LOOP_ACTIVE,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorLoopActive
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_LOOP_DIRECTION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorLoopDirection
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_LOOP_TIME,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorLoopTime
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_LOOP_START_ENHANCED_HUE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorLoopStartEnhancedHue
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_LOOP_STORED_ENHANCED_HUE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorLoopStoredEnhancedHue
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_COLOR_CAPABILITIES,
      ZCL_DATATYPE_BITMAP16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_ColorCapabilities
    }
  },
#ifdef ZLL_HW_LED_LAMP
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_1_X,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary1X
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_1_Y,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary1Y
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_1_INTENSITY,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary1Intensity
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_2_X,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary2X
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_2_Y,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary2Y
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_2_INTENSITY,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary2Intensity
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_3_X,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary3X
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_3_Y,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary3Y
    }
  },
  {
    ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
    { // Attribute record
      ATTRID_LIGHTING_COLOR_CONTROL_PRIMARY_3_INTENSITY,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclColor_Primary3Intensity
    }
  },
#endif //ZLL_HW_LED_LAMP
#endif //ZCL_COLOR_CTRL
};

uint8 CONST zclRGBLedBulb_NumAttributes = ( sizeof(zclRGBLedBulb_Attrs) / sizeof(zclRGBLedBulb_Attrs[0]) );

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
const cId_t zclRGBLedBulb_InClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_GROUPS,
  ZCL_CLUSTER_ID_GEN_SCENES,
  ZCL_CLUSTER_ID_GEN_ON_OFF,
#ifdef ZCL_LEVEL_CTRL
  ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL,
#endif
#ifdef ZCL_COLOR_CTRL
  ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
#endif
  // RGBLedBulb_TODO: Add application specific Input Clusters Here. 
  //       See zcl.h for Cluster ID definitions
  
};
#define ZCLRGBLedBulb_MAX_INCLUSTERS   (sizeof(zclRGBLedBulb_InClusterList) / sizeof(zclRGBLedBulb_InClusterList[0]))


const cId_t zclRGBLedBulb_OutClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  // RGBLedBulb_TODO: Add application specific Output Clusters Here. 
  //       See zcl.h for Cluster ID definitions
};
#define ZCLRGBLedBulb_MAX_OUTCLUSTERS  (sizeof(zclRGBLedBulb_OutClusterList) / sizeof(zclRGBLedBulb_OutClusterList[0]))


SimpleDescriptionFormat_t zclRGBLedBulb_SimpleDesc =
{
  RGBLedBulb_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                     //  uint16 AppProfId;
  // RGBLedBulb_TODO: Replace ZCL_HA_DEVICEID_ON_OFF_LIGHT with application specific device ID
  ZCL_HA_DEVICEID_SIMPLE_SENSOR,          //  uint16 AppDeviceId; 
  RGBLedBulb_DEVICE_VERSION,            //  int   AppDevVer:4;
  RGBLedBulb_FLAGS,                     //  int   AppFlags:4;
  ZCLRGBLedBulb_MAX_INCLUSTERS,         //  byte  AppNumInClusters;
  (cId_t *)zclRGBLedBulb_InClusterList, //  byte *pAppInClusterList;
  ZCLRGBLedBulb_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
  (cId_t *)zclRGBLedBulb_OutClusterList //  byte *pAppInClusterList;
};

// Added to include ZLL Target functionality
#if defined ( BDB_TL_INITIATOR ) || defined ( BDB_TL_TARGET )
bdbTLDeviceInfo_t tlRGBLedBulb_DeviceInfo =
{
  RGBLedBulb_ENDPOINT,                  //uint8 endpoint;
  ZCL_HA_PROFILE_ID,                        //uint16 profileID;
  // RGBLedBulb_TODO: Replace ZCL_HA_DEVICEID_ON_OFF_LIGHT with application specific device ID
  ZCL_HA_DEVICEID_SIMPLE_SENSOR,          //uint16 deviceID;
  RGBLedBulb_DEVICE_VERSION,                    //uint8 version;
  RGBLedBulb_NUM_GRPS                   //uint8 grpIdCnt;
};
#endif

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */
  
void zclRGBLedBulb_ResetAttributesToDefaultValues(void)
{
  int i;
  
  zclRGBLedBulb_LocationDescription[0] = 16;
  for (i = 1; i <= 16; i++)
  {
    zclRGBLedBulb_LocationDescription[i] = ' ';
  }
  
  zclRGBLedBulb_PhysicalEnvironment = PHY_UNSPECIFIED_ENV;
  zclRGBLedBulb_DeviceEnable = DEVICE_ENABLED;
  
#ifdef ZCL_IDENTIFY
  zclRGBLedBulb_IdentifyTime = 0;
#endif
  
  /* RGBLedBulb_TODO: initialize cluster attribute variables. */
}

/****************************************************************************
****************************************************************************/


