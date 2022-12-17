#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "MT_SYS.h"

#include "nwk_util.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_lighting.h"
#include "zcl_ha.h"
#include "zcl_diagnostic.h"
#include "zcl_RGBLedBulb.h"
#include "zll_effects_ctrl.h"
#include "zcl.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "gp_interface.h"

#include "hal_adc.h"
#include "hal_timer.h"

#if defined ( INTER_PAN )
#if defined ( BDB_TL_INITIATOR )
  #include "bdb_touchlink_initiator.h"
#endif // BDB_TL_INITIATOR
#if defined ( BDB_TL_TARGET )
  #include "bdb_touchlink_target.h"
#endif // BDB_TL_TARGET
#endif // INTER_PAN

#if defined ( BDB_TL_INITIATOR ) || defined ( BDB_TL_TARGET )
  #include "bdb_touchlink.h"
#endif

#include "onboard.h"

/* HAL */
//#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"

// my library
#include "uart.h"
#include "bitmasks.h"
#include "delay.h"
#include "pwm.h"
/*********************************************************************
 * MACROS
 */


/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zclRGBLedBulb_TaskID;
//int16 zclRGBLedBulb_MeasuredValue;
afAddrType_t zclRGBLedBulb_DstAddr;
 
float HumidityValue;
volatile uint8 powerSwCounting = 0;
char dat[20];

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8 SeqNum = 0;

uint8 giGenAppScreenMode = GENERIC_MAINMODE;   // display the main screen mode first

uint8 gPermitDuration = 0;    // permit joining default to disabled

devStates_t zclRGBLedBulb_NwkState = DEV_INIT;


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclRGBLedBulb_HandleKeys( byte shift, byte keys );
static void zclRGBLedBulb_BasicResetCB( void );
static void zclRGBLedBulb_IdentifyCB( zclIdentify_t *pCmd );
static void zclRGBLedBulb_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void zclRGBLedBulb_OnOffCB( uint8 cmd );
static void zclRGBLedBulb_OnOff_OffWithEffectCB( zclOffWithEffect_t *pCmd );
static void zclRGBLedBulb_OnOff_OnWithRecallGlobalSceneCB( void );
static void zclRGBLedBulb_OnOff_OnWithTimedOffCB( zclOnWithTimedOff_t *pCmd );
static void zclRGBLedBulb_ProcessOnWithTimedOffTimer( void );
static void zclRGBLedBulb_IdentifyEffectCB( zclIdentifyTriggerEffect_t *pCmd );

static void zclRGBLedBulb_ProcessIdentifyTimeChange( uint8 endpoint );
static void zclRGBLedBulb_BindNotification( bdbBindNotificationData_t *data );
#if ( defined ( BDB_TL_TARGET ) && (BDB_TOUCHLINK_CAPABILITY_ENABLED == TRUE) )
static void zclRGBLedBulb_ProcessTouchlinkTargetEnable( uint8 enable );
#endif

static void zclRGBLedBulb_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);


// app display functions
//static void zclRGBLedBulb_LcdDisplayUpdate( void );
#ifdef LCD_SUPPORTED
static void zclRGBLedBulb_LcdDisplayMainMode( void );
static void zclRGBLedBulb_LcdDisplayHelpMode( void );
#endif

// Functions to process ZCL Foundation incoming Command/Response messages
static void zclRGBLedBulb_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static uint8 zclRGBLedBulb_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
static uint8 zclRGBLedBulb_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zclRGBLedBulb_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#ifdef ZCL_DISCOVER
static uint8 zclRGBLedBulb_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 zclRGBLedBulb_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 zclRGBLedBulb_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg );
#endif

static void zclSampleApp_BatteryWarningCB( uint8 voltLevel);

// Application Function
static void zclRGBLedBulb_ReportLed( void );
static void zclRGBLedBulb_DetectShortRST( void );
void zclRGBLedBulb_LeaveNetwork( void );

// This callback is called to process attribute not handled in ZCL
static ZStatus_t zclRGBLedBulb_AttrReadWriteCB( uint16 clusterId, uint16 attrId,
                                       uint8 oper, uint8 *pValue, uint16 *pLen );
static uint8 zclRGBLedBulb_SceneStoreCB( zclSceneReq_t *pReq );
static void zclRGBLedBulb_SceneRecallCB( zclSceneReq_t *pReq );
/*********************************************************************
 * STATUS STRINGS
 */
#ifdef LCD_SUPPORTED
const char sDeviceName[]   = "  Generic App";
const char sClearLine[]    = " ";
const char sSwRGBLedBulb[]      = "SW1:GENAPP_TODO";  // RGBLedBulb_TODO
const char sSwBDBMode[]     = "SW2: Start BDB";
char sSwHelp[]             = "SW4: Help       ";  // last character is * if NWK open
#endif

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclRGBLedBulb_CmdCallbacks =
{
  zclRGBLedBulb_BasicResetCB,             // Basic Cluster Reset command
  zclRGBLedBulb_IdentifyEffectCB,                                   // Identify Trigger Effect command
  zclRGBLedBulb_OnOffCB,                                   // On/Off cluster commands
  zclRGBLedBulb_OnOff_OffWithEffectCB,                                   // On/Off cluster enhanced command Off with Effect
  zclRGBLedBulb_OnOff_OnWithRecallGlobalSceneCB,                                   // On/Off cluster enhanced command On with Recall Global Scene
  zclRGBLedBulb_OnOff_OnWithTimedOffCB,                                   // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_LEVEL_CTRL
  zclLevel_MoveToLevelCB,                 // Level Control Move to Level command
  zclLevel_MoveCB,                        // Level Control Move command
  zclLevel_StepCB,                        // Level Control Step command
  zclLevel_StopCB,                        // Level Control Stop command
#endif
#ifdef ZCL_GROUPS
  NULL,                                   // Group Response commands
#endif
#ifdef ZCL_SCENES
  zclRGBLedBulb_SceneStoreCB,            // Scene Store Request command
  zclRGBLedBulb_SceneRecallCB,           // Scene Recall Request command
  NULL,                                  // Scene Response command
#endif
#ifdef ZCL_ALARMS
  NULL,                                  // Alarm (Response) commands
#endif
#ifdef SE_UK_EXT
  NULL,                                  // Get Event Log command
  NULL,                                  // Publish Event Log command
#endif
  NULL,                                  // RSSI Location command
  NULL                                   // RSSI Location Response command
};

/*********************************************************************
 * RGBLedBulb_TODO: Add other callback structures for any additional application specific 
 *       Clusters being used, see available callback structures below.
 *
 *       bdbTL_AppCallbacks_t 
 *       zclApplianceControl_AppCallbacks_t 
 *       zclApplianceEventsAlerts_AppCallbacks_t 
 *       zclApplianceStatistics_AppCallbacks_t 
 *       zclElectricalMeasurement_AppCallbacks_t 
 *       zclGeneral_AppCallbacks_t 
 *       zclGp_AppCallbacks_t 
 *       zclHVAC_AppCallbacks_t 
 *       zclLighting_AppCallbacks_t 
 *       zclMS_AppCallbacks_t 
 *       zclPollControl_AppCallbacks_t 
 *       zclPowerProfile_AppCallbacks_t 
 *       zclSS_AppCallbacks_t  
 *
 */
static zclLighting_AppCallbacks_t zclRGBLedBulb_LightingCmdCBs =
{
  zclColor_MoveToHueCB,   //Move To Hue Command
  zclColor_MoveHueCB,   //Move Hue Command
  zclColor_StepHueCB,   //Step Hue Command
  zclColor_MoveToSaturationCB,   //Move To Saturation Command
  zclColor_MoveSaturationCB,   //Move Saturation Command
  zclColor_StepSaturationCB,   //Step Saturation Command
  zclColor_MoveToHueAndSaturationCB,   //Move To Hue And Saturation  Command
  zclColor_MoveToColorCB, // Move To Color Command
  zclColor_MoveColorCB,   // Move Color Command
  zclColor_StepColorCB,   // STEP To Color Command
  NULL,                                     // Move To Color Temperature Command
  zclColor_EnhMoveToHueCB,// Enhanced Move To Hue
  zclColor_MoveEnhHueCB,  // Enhanced Move Hue;
  zclColor_StepEnhHueCB,  // Enhanced Step Hue;
  zclColor_MoveToEnhHueAndSaturationCB, // Enhanced Move To Hue And Saturation;
  zclColor_SetColorLoopCB, // Color Loop Set Command
  zclColor_StopCB,        // Stop Move Step;
};
/*********************************************************************
 * @fn          zclRGBLedBulb_Init
 *
 * @brief       Initialization function for the zclGeneral layer.
 *
 * @param       none
 *
 * @return      none
 */
void zclRGBLedBulb_Init( byte task_id )
{
  zclRGBLedBulb_TaskID = task_id;

  // This app is part of the Home Automation Profile
  bdb_RegisterSimpleDescriptor( &zclRGBLedBulb_SimpleDesc );
  
  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( RGBLedBulb_ENDPOINT, &zclRGBLedBulb_CmdCallbacks );
  
  // RGBLedBulb_TODO: Register other cluster command callbacks here
  zclLighting_RegisterCmdCallbacks( RGBLedBulb_ENDPOINT, &zclRGBLedBulb_LightingCmdCBs );
  
#ifdef ZLL_HW_LED_LAMP
  HalTimer1Init(0);
#endif //ZLL_HW_LED_LAMP
    
  zllEffects_Init(zclRGBLedBulb_TaskID, zclRGBLedBulb_OnOffCB);
  
#ifdef ZCL_LEVEL_CTRL
  zclLevel_init(zclRGBLedBulb_TaskID, zclRGBLedBulb_OnOffCB);
#endif //ZCL_LEVEL_CTRL
  
#ifdef ZCL_COLOR_CTRL
  // Register the ZCL Lighting Cluster Library callback functions
  zclLighting_RegisterCmdCallbacks( RGBLedBulb_ENDPOINT, &zclRGBLedBulb_LightingCmdCBs );
  zclColor_init(zclRGBLedBulb_TaskID);
#endif //#ifdef ZCL_COLOR_CTRL
  
  // Register the application's attribute list
  zcl_registerAttrList( RGBLedBulb_ENDPOINT, zclRGBLedBulb_NumAttributes, zclRGBLedBulb_Attrs );

  // Register the application's callback function to read the Scene Count attribute.
  zcl_registerReadWriteCB( RGBLedBulb_ENDPOINT, zclRGBLedBulb_AttrReadWriteCB, NULL );
  
  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( zclRGBLedBulb_TaskID );

#ifdef ZCL_DISCOVER
  // Register the application's command list
  zcl_registerCmdList( RGBLedBulb_ENDPOINT, zclCmdsArraySize, zclRGBLedBulb_Cmds );
#endif

  // Register low voltage NV memory protection application callback
  RegisterVoltageWarningCB( zclSampleApp_BatteryWarningCB );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( zclRGBLedBulb_TaskID );
  
  zclRGBLedBulb_OnOffCB( zclRGBLedBulb_OnOff );
  
  bdb_RegisterCommissioningStatusCB( zclRGBLedBulb_ProcessCommissioningStatus );
  bdb_RegisterIdentifyTimeChangeCB( zclRGBLedBulb_ProcessIdentifyTimeChange );
  bdb_RegisterBindNotificationCB( zclRGBLedBulb_BindNotification );

#if ( defined ( BDB_TL_TARGET ) && (BDB_TOUCHLINK_CAPABILITY_ENABLED == TRUE) )
  bdb_RegisterTouchlinkTargetEnableCB( zclRGBLedBulb_ProcessTouchlinkTargetEnable );
#endif

#ifdef ZCL_DIAGNOSTIC
  // Register the application's callback function to read/write attribute data.
  // This is only required when the attribute data format is unknown to ZCL.
  zcl_registerReadWriteCB( RGBLedBulb_ENDPOINT, zclDiagnostic_ReadWriteAttrCB, NULL );

  if ( zclDiagnostic_InitStats() == ZSuccess )
  {
    // Here the user could start the timer to save Diagnostics to NV
  }
#endif


#ifdef LCD_SUPPORTED
  HalLcdWriteString ( (char *)sDeviceName, HAL_LCD_LINE_3 );
#endif  // LCD_SUPPORTED  

// Application
  
//  powerSwCounting++;
//  osal_nv_write(NV_PW_SW_COUTING_ID, 0, 1, &powerSwCounting);
  //osal_nv_write(NV_DIYRuZRT_RELAY_STATE_ID, 0, 1, &RELAY_STATE);
  
      // P0.2,3 UART; P0.4,5,6 ADC; P0.7, P1.0,1 PWM
  P0SEL |= BV(6)|BV(5)|BV(4)|BV(3)|BV(2);
  APCFG |= BV(6)|BV(5)|BV(4);
  P0INP |= BV(4);
  
  // Power off interrupt
  P0SEL &= ~BV(0);
  P0DIR &= (~BV(0));
  P0INP &= ~BV(0);
  P2INP |= BV(5);
  
  PICTL |= BV(0);
  P0IEN |= BV(0);
  IEN1 |= BV(5);
  P0IFG = 0;

  
#ifdef UART_DEBUG 
  UART_Init();
#endif
  //zclRGBLedBulb_DetectShortRST();
  
  HalAdcInit();
  HalAdcSetReference(HAL_ADC_REF_125V);
  
//  PWM_Init();
  #ifdef UART_DEBUG
  UART_String("start");
#endif
  
  zclColor_init(zclRGBLedBulb_TaskID);
//  hwLight_UpdateOnOff( LIGHT_OFF );
//  halTimer1SetChannelDuty (RED_LED, 1000);
//  halTimer1SetChannelDuty (GREEN_LED, 1000);
//  halTimer1SetChannelDuty (BLUE_LED, 1000);
  //bdb_StartCommissioning(BDB_COMMISSIONING_MODE_PARENT_LOST);
  bdb_StartCommissioning(
          BDB_COMMISSIONING_MODE_NWK_FORMATION | 
          BDB_COMMISSIONING_MODE_NWK_STEERING | 
          BDB_COMMISSIONING_MODE_FINDING_BINDING | 
          BDB_COMMISSIONING_MODE_INITIATOR_TL
        );
  
  //osal_start_reload_timer( zclRGBLedBulb_TaskID, RGBLedBulb_REPORTING_EVT, 3000 );

//  bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING |
//                         BDB_COMMISSIONING_MODE_FINDING_BINDING);
}

/*********************************************************************
 * @fn          zclSample_event_loop
 *
 * @brief       Event Loop Processor for zclGeneral.
 *
 * @param       none
 *
 * @return      none
 */
uint16 zclRGBLedBulb_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( zclRGBLedBulb_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          zclRGBLedBulb_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          zclRGBLedBulb_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_STATE_CHANGE:
          zclRGBLedBulb_NwkState = (devStates_t)(MSGpkt->hdr.status);

          // now on the network
          if ( (zclRGBLedBulb_NwkState == DEV_ZB_COORD) ||
               (zclRGBLedBulb_NwkState == DEV_ROUTER)   ||
               (zclRGBLedBulb_NwkState == DEV_END_DEVICE) )
          {
            giGenAppScreenMode = GENERIC_MAINMODE;
//            zclRGBLedBulb_LcdDisplayUpdate();
          }
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & RGBLedBulb_MAIN_SCREEN_EVT )
  {
    giGenAppScreenMode = GENERIC_MAINMODE;


    return ( events ^ RGBLedBulb_MAIN_SCREEN_EVT );
  }
  
#if ZG_BUILD_ENDDEVICE_TYPE    
  if ( events & RGBLedBulb_END_DEVICE_REJOIN_EVT )
  {
    bdb_ZedAttemptRecoverNwk();
    return ( events ^ RGBLedBulb_END_DEVICE_REJOIN_EVT );
  }
#endif

  /* RGBLedBulb_TODO: handle app events here */
  
  if ( events & RGBLedBulb_EVT_LONG )
  {
    if (P0_0 == 1)
    {
      UART_String("zoooo");
      powerSwCounting = 0;
      if ( bdbAttributes.bdbNodeIsOnANetwork )
      {
        zclRGBLedBulb_LeaveNetwork();
      }
      else 
      {
        bdb_StartCommissioning(
          BDB_COMMISSIONING_MODE_NWK_FORMATION | 
          BDB_COMMISSIONING_MODE_NWK_STEERING | 
          BDB_COMMISSIONING_MODE_FINDING_BINDING | 
          BDB_COMMISSIONING_MODE_INITIATOR_TL
        );
      }
    } 
    else
      osal_start_timerEx(zclRGBLedBulb_TaskID, RGBLedBulb_EVT_LONG, 100);
    
    return ( events ^ RGBLedBulb_EVT_LONG );
  }
  
  if ( events & RGBLedBulb_REPORTING_EVT )
  {
    // toggle LED 2 state, start another timer for 500ms
    //HalLedSet ( HAL_LED_2, HAL_LED_MODE_TOGGLE );
    //osal_start_timerEx( zclRGBLedBulb_TaskID, RGBLedBulb_REPORTING_EVT, 3000 );
    //P0_6 = ~P0_6;
//    DHT22_Measure();
//    zclRGBLedBulb_ReportTemp();

    sprintf(dat,"ADC 1: %d", HalAdcRead(HAL_ADC_CHANNEL_4, HAL_ADC_RESOLUTION_14));
    UART_String(dat);
    
    sprintf(dat,"ADC 2: %d", HalAdcRead(HAL_ADC_CHANNEL_5, HAL_ADC_RESOLUTION_14));
    UART_String(dat);

    sprintf(dat,"ADC 3: %d", HalAdcRead(HAL_ADC_CHANNEL_6, HAL_ADC_RESOLUTION_14));
    UART_String(dat);
    return ( events ^ RGBLedBulb_REPORTING_EVT );
  }
  
  
  if ( events & RGBLedBulb_RST_COUNTING_EVT )
  {
    powerSwCounting = 0;
    UART_String("rst");
    return ( events ^ RGBLedBulb_RST_COUNTING_EVT );
  }
  
    if ( events & RGBLedBulb_EFFECT_PROCESS_EVT )
  {
    zllEffects_ProcessEffect();
    return ( events ^ RGBLedBulb_EFFECT_PROCESS_EVT );
  }

  if ( events & RGBLedBulb_ON_TIMED_OFF_TIMER_EVT )
  {
    zclRGBLedBulb_ProcessOnWithTimedOffTimer();
    return ( events ^ RGBLedBulb_ON_TIMED_OFF_TIMER_EVT );
  }

  //zclRGBLedBulb_process(&events);
 #ifdef ZCL_LEVEL_CTRL
    //update the level
    zclLevel_process(&events);
#endif //ZCL_COLOR_CTRL

#ifdef ZCL_COLOR_CTRL
    //update the color
    zclColor_process(&events);
    zclColor_processColorLoop(&events);
#endif //ZCL_COLOR_CTRL


  
  // Discard unknown events
  return 0;
}


/*********************************************************************
 * @fn      zclRGBLedBulb_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_5
 *                 HAL_KEY_SW_4
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void zclRGBLedBulb_HandleKeys( byte shift, byte keys )
{
//  if ( keys & HAL_KEY_SW_1 )
//  {
//    if(ledState == 1){
//      ledState = 0;
//    } else 
//    {
//      ledState = 1;
//    }
//    zclRGBLedBulb_ReportLed();
//  }
//  // Start the BDB commissioning method
//  if ( keys & HAL_KEY_SW_2 )
//  {
//    giGenAppScreenMode = GENERIC_MAINMODE;
//
//    bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_FORMATION | BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING | BDB_COMMISSIONING_MODE_INITIATOR_TL);
//  }
//  if ( keys & HAL_KEY_SW_3 )
//  {
//    giGenAppScreenMode = GENERIC_MAINMODE;
//  
//    // touchlink target commissioning, if enabled  
//#if ( defined ( BDB_TL_TARGET ) && (BDB_TOUCHLINK_CAPABILITY_ENABLED == TRUE) )
//    bdb_StartCommissioning(BDB_COMMISSIONING_MODE_FINDING_BINDING);
//    touchLinkTarget_EnableCommissioning( 30000 );
//#endif
//    
//  }
//  if ( keys & HAL_KEY_SW_4 )
//  {
//    
//   giGenAppScreenMode = giGenAppScreenMode ? GENERIC_MAINMODE : GENERIC_HELPMODE;
//#ifdef LCD_SUPPORTED
//    HalLcdWriteString( (char *)sClearLine, HAL_LCD_LINE_2 );
//#endif
//    
//  }
//  if ( keys & HAL_KEY_SW_5 )
//  {
//    bdb_resetLocalAction();
//  }
}


/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessCommissioningStatus
 *
 * @brief   Callback in which the status of the commissioning process are reported
 *
 * @param   bdbCommissioningModeMsg - Context message of the status of a commissioning process
 *
 * @return  none
 */
static void zclRGBLedBulb_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg)
{
  switch(bdbCommissioningModeMsg->bdbCommissioningMode)
  {
    case BDB_COMMISSIONING_FORMATION:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //After formation, perform nwk steering again plus the remaining commissioning modes that has not been process yet
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | bdbCommissioningModeMsg->bdbRemainingCommissioningModes);
      }
      else
      {
        //Want to try other channels?
        //try with bdb_setChannelAttribute
      }
    break;
    case BDB_COMMISSIONING_NWK_STEERING:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //YOUR JOB:
        //We are on the nwk, what now?
      }
      else
      {
        //See the possible errors for nwk steering procedure
        //No suitable networks found
        //Want to try other channels?
        //try with bdb_setChannelAttribute
      }
    break;
    case BDB_COMMISSIONING_FINDING_BINDING:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //YOUR JOB:
      }
      else
      {
        //YOUR JOB:
        //retry?, wait for user interaction?
      }
    break;
    case BDB_COMMISSIONING_INITIALIZATION:
      //Initialization notification can only be successful. Failure on initialization
      //only happens for ZED and is notified as BDB_COMMISSIONING_PARENT_LOST notification

      //YOUR JOB:
      //We are on a network, what now?

    break;
#if ZG_BUILD_ENDDEVICE_TYPE    
    case BDB_COMMISSIONING_PARENT_LOST:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_NETWORK_RESTORED)
      {
        //We did recover from losing parent
      }
      else
      {
        //Parent not found, attempt to rejoin again after a fixed delay
        osal_start_timerEx(zclRGBLedBulb_TaskID, RGBLedBulb_END_DEVICE_REJOIN_EVT, RGBLedBulb_END_DEVICE_REJOIN_DELAY);
      }
    break;
#endif 
  }
}

/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessIdentifyTimeChange
 *
 * @brief   Called to process any change to the IdentifyTime attribute.
 *
 * @param   endpoint - in which the identify has change
 *
 * @return  none
 */
static void zclRGBLedBulb_ProcessIdentifyTimeChange( uint8 endpoint )
{
  if ( zclRGBLedBulb_IdentifyTime > 0 )
  {
    zclRGBLedBulb_IdentifyTime--;
    zllEffects_Blink();
    osal_start_timerEx( zclRGBLedBulb_TaskID, RGBLedBulb_IDENTIFY_TIMEOUT_EVT, 1000 );
  }
}

/*********************************************************************
 * @fn      zclRGBLedBulb_BindNotification
 *
 * @brief   Called when a new bind is added.
 *
 * @param   data - pointer to new bind data
 *
 * @return  none
 */
static void zclRGBLedBulb_BindNotification( bdbBindNotificationData_t *data )
{
  // RGBLedBulb_TODO: process the new bind information
}


/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessTouchlinkTargetEnable
 *
 * @brief   Called to process when the touchlink target functionality
 *          is enabled or disabled
 *
 * @param   none
 *
 * @return  none
 */
#if ( defined ( BDB_TL_TARGET ) && (BDB_TOUCHLINK_CAPABILITY_ENABLED == TRUE) )
static void zclRGBLedBulb_ProcessTouchlinkTargetEnable( uint8 enable )
{
  if ( enable )
  {
    HalLedSet ( HAL_LED_1, HAL_LED_MODE_ON );
  }
  else
  {
    HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );
  }
}
#endif
/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessOnWithTimedOffTimer
 *
 * @brief   Called to process On with Timed Off attributes changes over time.
 *
 * @param   none
 *
 * @return  none
 */
static void zclRGBLedBulb_ProcessOnWithTimedOffTimer( void )
{
  if ( ( zclRGBLedBulb_OnOff == LIGHT_ON ) && ( zclRGBLedBulb_OnTime > 0 ) )
  {
    zclRGBLedBulb_OnTime--;
    if ( zclRGBLedBulb_OnTime <= 0 )
    {
      zclRGBLedBulb_OffWaitTime = 0x00;
      zclRGBLedBulb_OnOffCB( COMMAND_OFF );
    }
  }
  if ( ( zclRGBLedBulb_OnOff == LIGHT_OFF ) && ( zclRGBLedBulb_OffWaitTime > 0 ) )
  {
    zclRGBLedBulb_OffWaitTime--;
    if ( zclRGBLedBulb_OffWaitTime <= 0 )
    {
      osal_stop_timerEx( zclRGBLedBulb_TaskID, RGBLedBulb_ON_TIMED_OFF_TIMER_EVT);
      return;
    }
  }

  if ( ( zclRGBLedBulb_OnTime > 0 ) || ( zclRGBLedBulb_OffWaitTime > 0 ) )
  {
    osal_start_timerEx( zclRGBLedBulb_TaskID, RGBLedBulb_ON_TIMED_OFF_TIMER_EVT, 100 );
  }
}

/*********************************************************************
 * @fn      zclRGBLedBulb_AttrReadWriteCB
 *
 * @brief   Read/write callbackfor read/wtire attrs tha have NULL dataPtr
 *
 *          Note: This function gets called only when the pointer
 *                'dataPtr' to the Scene Count attribute value is
 *                NULL in the attribute database registered with
 *                the ZCL.
 *
 * @param   clusterId - cluster that attribute belongs to
 * @param   attrId - attribute to be read or written
 * @param   oper - ZCL_OPER_LEN, ZCL_OPER_READ, or ZCL_OPER_WRITE
 * @param   pValue - pointer to attribute value
 * @param   pLen - length of attribute value read
 *
 * @return  status
 */
ZStatus_t zclRGBLedBulb_AttrReadWriteCB( uint16 clusterId, uint16 attrId,
                                       uint8 oper, uint8 *pValue, uint16 *pLen )
{
  ZStatus_t status = ZCL_STATUS_SUCCESS;

#if defined ZCL_SCENES
  //SceneCount Attr
  if( (clusterId == ZCL_CLUSTER_ID_GEN_SCENES) &&
     (attrId == ATTRID_SCENES_COUNT) )
  {
    status = zclGeneral_ReadSceneCountCB(clusterId, attrId, oper, pValue, pLen);
  } else
#endif //ZCL_SCENES
  //IdentifyTime Attr
  if( (clusterId == ZCL_CLUSTER_ID_GEN_IDENTIFY) &&
     (attrId == ATTRID_IDENTIFY_TIME) )
  {
    switch ( oper )
    {
      case ZCL_OPER_LEN:
        *pLen = 2; // uint16
        break;

      case ZCL_OPER_READ:
        pValue[0] = LO_UINT16( zclRGBLedBulb_IdentifyTime );
        pValue[1] = HI_UINT16( zclRGBLedBulb_IdentifyTime );

        if ( pLen != NULL )
        {
          *pLen = 2;
        }
        break;

      case ZCL_OPER_WRITE:
      {
        zclIdentify_t cmd;
        cmd.identifyTime = BUILD_UINT16( pValue[0], pValue[1] );

        zclRGBLedBulb_IdentifyCB( &cmd );

        break;
      }

      default:
        status = ZCL_STATUS_SOFTWARE_FAILURE; // should never get here!
        break;
    }
  }
  else
  {
    status = ZCL_STATUS_SOFTWARE_FAILURE; // should never get here!
  }

  return ( status );
}
/*********************************************************************
 * @fn      zclRGBLedBulb_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zclRGBLedBulb_BasicResetCB( void )
{

  /* RGBLedBulb_TODO: remember to update this function with any
     application-specific cluster attribute variables */
  
  zclRGBLedBulb_ResetAttributesToDefaultValues();
  
}
/*********************************************************************
 * @fn      zclSampleApp_BatteryWarningCB
 *
 * @brief   Called to handle battery-low situation.
 *
 * @param   voltLevel - level of severity
 *
 * @return  none
 */
void zclSampleApp_BatteryWarningCB( uint8 voltLevel )
{
  if ( voltLevel == VOLT_LEVEL_CAUTIOUS )
  {
    // Send warning message to the gateway and blink LED
  }
  else if ( voltLevel == VOLT_LEVEL_BAD )
  {
    // Shut down the system
  }
}
/*********************************************************************
 * @fn      zclRGBLedBulb_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void zclRGBLedBulb_IdentifyCB( zclIdentify_t *pCmd )
{
  zclRGBLedBulb_IdentifyTime = pCmd->identifyTime;
  zclRGBLedBulb_ProcessIdentifyTimeChange(RGBLedBulb_ENDPOINT);
}


/*********************************************************************
 * @fn      zclRGBLedBulb_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void zclRGBLedBulb_IdentifyEffectCB( zclIdentifyTriggerEffect_t *pCmd )
{
//  HalLcdWriteStringValue( "IdentifyEffId", pCmd->effectId, 16, HAL_LCD_LINE_1 );
  zllEffects_InitiateEffect( ZCL_CLUSTER_ID_GEN_IDENTIFY, COMMAND_IDENTIFY_TRIGGER_EFFECT,
                                 pCmd->effectId, pCmd->effectVariant );
}

/*********************************************************************
 * @fn      zclRGBLedBulb_IdentifyQueryRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Query Response Command for this application.
 *
 * @param   srcAddr - requestor's address
 * @param   timeout - number of seconds to identify yourself (valid for query response)
 *
 * @return  none
 */
static void zclRGBLedBulb_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp )
{
  // Query Response (with timeout value)
  (void)pRsp;
}

/*********************************************************************
 * @fn      zclRGBLedBulb_OnOffCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an On/Off Command for this application.
 *
 * @param   cmd - COMMAND_ON, COMMAND_OFF, or COMMAND_TOGGLE
 *
 * @return  none
 */
static void zclRGBLedBulb_OnOffCB( uint8 cmd )
{
  // Turn on the light
  if ( cmd == COMMAND_ON )
  {
    UART_String("ON");
    zclRGBLedBulb_OnOff = LIGHT_ON;
    zclRGBLedBulb_GlobalSceneCtrl = TRUE;
    if ( zclRGBLedBulb_OnTime == 0 )
    {
      zclRGBLedBulb_OffWaitTime = 0;
    }
  }

  // Turn off the light
  else if ( cmd == COMMAND_OFF )
  {
    UART_String("OFF");
    zclRGBLedBulb_OnOff = LIGHT_OFF;
    //zclRGBLedBulb_GlobalSceneCtrl = FALSE; //see ZLL spec 11-0037-03 6.6.1.2.1
    zclRGBLedBulb_OnTime = 0;
  }

  // Toggle the light
  else
  {
    if ( zclRGBLedBulb_OnOff == LIGHT_OFF )
    {
      zclRGBLedBulb_OnOff = LIGHT_ON;
      zclRGBLedBulb_GlobalSceneCtrl = TRUE;
      if ( zclRGBLedBulb_OnTime == 0 )
      {
        zclRGBLedBulb_OffWaitTime = 0;
      }
    }
    else
    {
      zclRGBLedBulb_OnOff = LIGHT_OFF;
      zclRGBLedBulb_OnTime = 0;
    }
  }

  hwLight_UpdateOnOff( zclRGBLedBulb_OnOff );

  zclRGBLedBulb_SceneValid = 0;
}


/*********************************************************************
 * @fn      zclRGBLedBulb_OffWithEffect
 *
 * @brief   Callback from the ZCL General Cluster Library when it
 *          received an Off with Effect Command for this application.
 *
 * @param   pCmd - Off with Effect parameters
 *
 * @return  none
 */
static void zclRGBLedBulb_OnOff_OffWithEffectCB( zclOffWithEffect_t *pCmd )
{
//  HalLcdWriteStringValueValue( "OffWithEff", pCmd->effectId, 16,
//                               pCmd->effectVariant, 16, HAL_LCD_LINE_1 );
  if( zclRGBLedBulb_GlobalSceneCtrl )
  {
    zclSceneReq_t req;
    req.scene = &zclRGBLedBulb_GlobalScene;

    zclRGBLedBulb_SceneStoreCB( &req );
    zclRGBLedBulb_GlobalSceneCtrl = FALSE;
  }
  zllEffects_InitiateEffect( ZCL_CLUSTER_ID_GEN_ON_OFF, COMMAND_OFF_WITH_EFFECT,
                                 pCmd->effectId, pCmd->effectVariant );
}

/*********************************************************************
 * @fn      zclRGBLedBulb_OnOff_OnWithRecallGlobalSceneCB
 *
 * @brief   Callback from the ZCL General Cluster Library when it
 *          received an On with Recall Global Scene Command for this application.
 *
 * @param   none
 *
 * @return  none
 */
static void zclRGBLedBulb_OnOff_OnWithRecallGlobalSceneCB( void )
{
  if( !zclRGBLedBulb_GlobalSceneCtrl )
  {
    zclSceneReq_t req;
    req.scene = &zclRGBLedBulb_GlobalScene;

    zclRGBLedBulb_SceneRecallCB( &req );

    zclRGBLedBulb_GlobalSceneCtrl = TRUE;
  }
  // If the GlobalSceneControl attribute is equal to TRUE, discard the command
}

/*********************************************************************
 * @fn      zclRGBLedBulb_OnOff_OnWithTimedOffCB
 *
 * @brief   Callback from the ZCL General Cluster Library when it
 *          received an On with Timed Off Command for this application.
 *
 * @param   pCmd - On with Timed Off parameters
 *
 * @return  none
 */
static void zclRGBLedBulb_OnOff_OnWithTimedOffCB( zclOnWithTimedOff_t *pCmd )
{
  if ( ( pCmd->onOffCtrl.bits.acceptOnlyWhenOn == TRUE )
      && ( zclRGBLedBulb_OnOff == LIGHT_OFF ) )
  {
    return;
  }

  if ( ( zclRGBLedBulb_OffWaitTime > 0 ) && ( zclRGBLedBulb_OnOff == LIGHT_OFF ) )
  {
    zclRGBLedBulb_OffWaitTime = MIN( zclRGBLedBulb_OffWaitTime, pCmd->offWaitTime );
  }
  else
  {
    uint16 maxOnTime = MAX( zclRGBLedBulb_OnTime, pCmd->onTime );
    zclRGBLedBulb_OnOffCB( COMMAND_ON );
    zclRGBLedBulb_OnTime = maxOnTime;
    zclRGBLedBulb_OffWaitTime = pCmd->offWaitTime;
  }

  if ( ( zclRGBLedBulb_OnTime < 0xFFFF ) && ( zclRGBLedBulb_OffWaitTime < 0xFFFF ) )
  {
    osal_start_timerEx( zclRGBLedBulb_TaskID, RGBLedBulb_ON_TIMED_OFF_TIMER_EVT, 100 );
  }
}

#define COLOR_SCN_X_Y_ATTRS_SIZE     ( sizeof(zclColor_CurrentX) + sizeof(zclColor_CurrentY) )
#define COLOR_SCN_HUE_SAT_ATTRS_SIZE ( sizeof(zclColor_EnhancedCurrentHue) + sizeof(zclColor_CurrentSaturation) )
#define COLOR_SCN_LOOP_ATTRS_SIZE    ( sizeof(zclColor_ColorLoopActive) + sizeof(zclColor_ColorLoopDirection) + sizeof(zclColor_ColorLoopTime) )
/*********************************************************************
 * @fn      zclRGBLedBulb_SceneStoreCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received a Scene Store Request Command for
 *          this application.
 *          Stores current attributes in the scene's extension fields.
 *          Extension field sets =
 *          {{Cluster ID 1, length 1, {extension field set 1}}, {{Cluster ID 2,
 *            length 2, {extension field set 2}}, ...}
 *
 * @param   pReq - pointer to a request holding scene data
 *
 * @return  TRUE if extField is filled out, FALSE if not filled
 *          and there is no need to save the scene
 */
static uint8 zclRGBLedBulb_SceneStoreCB( zclSceneReq_t *pReq )
{
  uint8 *pExt;

  pReq->scene->extLen = RGBLedBulb_SCENE_EXT_FIELD_SIZE;
  pExt = pReq->scene->extField;

  // Build an extension field for On/Off cluster
  *pExt++ = LO_UINT16( ZCL_CLUSTER_ID_GEN_ON_OFF );
  *pExt++ = HI_UINT16( ZCL_CLUSTER_ID_GEN_ON_OFF );
  *pExt++ = sizeof(zclRGBLedBulb_OnOff); // length

  // Store the value of onOff attribute
  *pExt++ = zclRGBLedBulb_OnOff;

#ifdef ZCL_LEVEL_CTRL
  // Build an extension field for Level Control cluster
  *pExt++ = LO_UINT16( ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL );
  *pExt++ = HI_UINT16( ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL );
  *pExt++ = sizeof(zclLevel_CurrentLevel); // length

  // Store the value of currentLevel attribute
  *pExt++ = zclLevel_CurrentLevel;
#endif //ZCL_LEVEL_CTRL

#ifdef ZCL_COLOR_CTRL
  // Build an extension field for Color Control cluster
  *pExt++ = LO_UINT16( ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL );
  *pExt++ = HI_UINT16( ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL );
  *pExt++ = COLOR_SCN_X_Y_ATTRS_SIZE + COLOR_SCN_HUE_SAT_ATTRS_SIZE + COLOR_SCN_LOOP_ATTRS_SIZE; // length

  // Restored color mode is determined by whether stored currentX, currentY values are 0.
  if ( zclColor_ColorMode == COLOR_MODE_CURRENT_X_Y )
  {
    *pExt++ = LO_UINT16( zclColor_CurrentX );
    *pExt++ = HI_UINT16( zclColor_CurrentX );
    *pExt++ = LO_UINT16( zclColor_CurrentY );
    *pExt++ = HI_UINT16( zclColor_CurrentY );
    pExt += COLOR_SCN_HUE_SAT_ATTRS_SIZE + COLOR_SCN_LOOP_ATTRS_SIZE; // ignore other parameters
  }
  else
  {
    // nullify currentX and currentY to mark hue/sat color mode
    osal_memset( pExt, 0x00, COLOR_SCN_X_Y_ATTRS_SIZE );
    pExt += COLOR_SCN_X_Y_ATTRS_SIZE;
    *pExt++ = LO_UINT16( zclColor_EnhancedCurrentHue );
    *pExt++ = HI_UINT16( zclColor_EnhancedCurrentHue );
    *pExt++ = zclColor_CurrentSaturation;
    *pExt++ = zclColor_ColorLoopActive;
    *pExt++ = zclColor_ColorLoopDirection;
    *pExt++ = LO_UINT16( zclColor_ColorLoopTime );
    *pExt++ = HI_UINT16( zclColor_ColorLoopTime );
  }
#endif //ZCL_COLOR_CTRL

  // Add more clusters here

  return ( TRUE );
}

/*********************************************************************
 * @fn      zclRGBLedBulb_SceneRecallCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received a Scene Recall Request Command for
 *          this application.
 *          Restores attributes values from the scene's extension fields.
 *          Extension field sets =
 *          {{Cluster ID 1, length 1, {extension field set 1}}, {{Cluster ID 2,
 *            length 2, {extension field set 2}}, ...}
 *
 * @param   pReq - pointer to a request holding scene data
 *
 * @return  none
 */
static void zclRGBLedBulb_SceneRecallCB( zclSceneReq_t *pReq )
{
  int8 remain;
  uint16 clusterID;
  uint8 *pExt = pReq->scene->extField;

  while ( pExt < pReq->scene->extField + pReq->scene->extLen )
  {
    clusterID =  BUILD_UINT16( pExt[0], pExt[1] );
    pExt += 2; // cluster ID
    remain = *pExt++;

    if ( clusterID == ZCL_CLUSTER_ID_GEN_ON_OFF )
    {
      // Update onOff attibute with the recalled value
      if ( remain > 0 )
      {
        zclRGBLedBulb_OnOffCB( *pExt++ ); // Apply the new value
        remain--;
      }
    }
#ifdef ZCL_LEVEL_CTRL
    else if ( clusterID == ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL )
    {
      // Update currentLevel attribute with the recalled value
      if ( remain > 0 )
      {
        zclLCMoveToLevel_t levelCmd;

        levelCmd.level = *pExt++;
        levelCmd.transitionTime = pReq->scene->transTime; // whole seconds only
        levelCmd.withOnOff = 0;
        zclLevel_MoveToLevelCB( &levelCmd );
        remain--;
      }
    }
#endif //ZCL_LEVEL_CTRL
#ifdef ZCL_COLOR_CTRL
    else if ( clusterID == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL )
    {
      // Update currentX and currentY attributes with the recalled values
      if ( remain >= COLOR_SCN_X_Y_ATTRS_SIZE )
      {
        zclCCMoveToColor_t colorCmd = {0};

        colorCmd.colorX = BUILD_UINT16( pExt[0], pExt[1] );
        colorCmd.colorY = BUILD_UINT16( pExt[2], pExt[3] );
        pExt += COLOR_SCN_X_Y_ATTRS_SIZE;
        remain -= COLOR_SCN_X_Y_ATTRS_SIZE;
        if ( ( colorCmd.colorX != 0 ) || ( colorCmd.colorY != 0 ) )
        {
          // COLOR_MODE_CURRENT_X_Y
          colorCmd.transitionTime = (10 * pReq->scene->transTime) + pReq->scene->transTime100ms; // in 1/10th seconds
          zclColor_MoveToColorCB( &colorCmd );
          // for non-zero X,Y other hue/sat and loop parameters are ignored (CCB 1683)
        }
        else if ( remain >= COLOR_SCN_HUE_SAT_ATTRS_SIZE )
        {
          // ENHANCED_COLOR_MODE_ENHANCED_CURRENT_HUE_SATURATION
          zclCCEnhancedMoveToHueAndSaturation_t cmd = {0};

          cmd.enhancedHue = BUILD_UINT16( pExt[0], pExt[1] );
          pExt += 2;
          cmd.saturation = *pExt++;
          cmd.transitionTime = (10 * pReq->scene->transTime) + pReq->scene->transTime100ms; // in 1/10th seconds
          zclColor_MoveToEnhHueAndSaturationCB( &cmd );
          remain -= COLOR_SCN_HUE_SAT_ATTRS_SIZE;

          if ( remain >= COLOR_SCN_LOOP_ATTRS_SIZE )
          {
            zclCCColorLoopSet_t newColorLoopSetCmd = {0};
            newColorLoopSetCmd.updateFlags.bits.action = TRUE;
            newColorLoopSetCmd.action = ( (*pExt++) ? LIGHTING_COLOR_LOOP_ACTION_ACTIVATE_FROM_ENH_CURR_HUE : LIGHTING_COLOR_LOOP_ACTION_DEACTIVATE );
            newColorLoopSetCmd.updateFlags.bits.direction = TRUE;
            newColorLoopSetCmd.direction = *pExt++;
            newColorLoopSetCmd.updateFlags.bits.time = TRUE;
            newColorLoopSetCmd.time = BUILD_UINT16( pExt[0], pExt[1] );
            pExt += 2;
            zclColor_SetColorLoopCB( &newColorLoopSetCmd );
            remain -= COLOR_SCN_LOOP_ATTRS_SIZE;
          }
        }
      }
    }
#endif //ZCL_COLOR_CTRL

    // Add more clusters here

    pExt += remain; // remain should be 0 if all extension fields are processed
  }

  zclRGBLedBulb_CurrentScene = pReq->scene->ID;
  zclRGBLedBulb_CurrentGroup = pReq->scene->groupID;
  zclRGBLedBulb_GlobalSceneCtrl = TRUE;
  SCENE_VALID();
}
/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void zclRGBLedBulb_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      zclRGBLedBulb_ProcessInReadRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_WRITE
    case ZCL_CMD_WRITE_RSP:
      zclRGBLedBulb_ProcessInWriteRspCmd( pInMsg );
      break;
#endif
    case ZCL_CMD_CONFIG_REPORT:
    case ZCL_CMD_CONFIG_REPORT_RSP:
    case ZCL_CMD_READ_REPORT_CFG:
    case ZCL_CMD_READ_REPORT_CFG_RSP:
    case ZCL_CMD_REPORT:
      //bdb_ProcessIncomingReportingMsg( pInMsg );
      break;
      
    case ZCL_CMD_DEFAULT_RSP:
      zclRGBLedBulb_ProcessInDefaultRspCmd( pInMsg );
      break;
#ifdef ZCL_DISCOVER
    case ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP:
      zclRGBLedBulb_ProcessInDiscCmdsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_CMDS_GEN_RSP:
      zclRGBLedBulb_ProcessInDiscCmdsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_ATTRS_RSP:
      zclRGBLedBulb_ProcessInDiscAttrsRspCmd( pInMsg );
      break;

    case ZCL_CMD_DISCOVER_ATTRS_EXT_RSP:
      zclRGBLedBulb_ProcessInDiscAttrsExtRspCmd( pInMsg );
      break;
#endif
    default:
      break;
  }

  if ( pInMsg->attrCmd )
    osal_mem_free( pInMsg->attrCmd );
}

#ifdef ZCL_READ
/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRGBLedBulb_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
    // Notify the originator of the results of the original read attributes
    // attempt and, for each successfull request, the value of the requested
    // attribute
  }

  return ( TRUE );
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRGBLedBulb_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;

  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < writeRspCmd->numAttr; i++ )
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }

  return ( TRUE );
}
#endif // ZCL_WRITE

/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRGBLedBulb_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  // Device is notified of the Default Response command.
  (void)pInMsg;

  return ( TRUE );
}

#ifdef ZCL_DISCOVER
/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessInDiscCmdsRspCmd
 *
 * @brief   Process the Discover Commands Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRGBLedBulb_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverCmdsCmdRsp_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverCmdsCmdRsp_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numCmd; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return ( TRUE );
}

/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessInDiscAttrsRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRGBLedBulb_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsRspCmd_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return ( TRUE );
}

/*********************************************************************
 * @fn      zclRGBLedBulb_ProcessInDiscAttrsExtRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Extended Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclRGBLedBulb_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsExtRsp_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsExtRsp_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return ( TRUE );
}
#endif // ZCL_DISCOVER


//static void zclRGBLedBulb_ReportLed( void )
//{
//  // ?????? ???????????
//  const uint8 NUM_ATTRIBUTES = 1;
//
//  zclReportCmd_t *pReportCmd;
//
//  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) +
//                              (NUM_ATTRIBUTES * sizeof(zclReport_t)));
//  if (pReportCmd != NULL) {
//    pReportCmd->numAttr = NUM_ATTRIBUTES;
//
//    pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
//    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
//    pReportCmd->attrList[0].attrData = (void *)(&ledState);
//    
//    zclRGBLedBulb_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
//    zclRGBLedBulb_DstAddr.addr.shortAddr = 0;
//    zclRGBLedBulb_DstAddr.endPoint = 1;
//
//    zcl_SendReportCmd(RGBLedBulb_ENDPOINT, &zclRGBLedBulb_DstAddr,
//                      ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd,
//                      ZCL_FRAME_CLIENT_SERVER_DIR, false, SeqNum++);
//  }
//
//  osal_mem_free(pReportCmd);
//}

//static void zclRGBLedBulb_DetectShortRST( void )
//{
//  //  turn on < 1.5s and turn off < 1.8s consequencely 3 time to go to <functional> (100uf & 56k ohm)
//  if(P0_1 == 1) // recent turn off?
//  {
//    //UART_String("reset detect");
//    if ( SUCCESS == osal_nv_item_init( NV_PW_SW_COUTING_ID, 1, &powerSwCounting ) ) {
//      osal_nv_read( NV_PW_SW_COUTING_ID, 0, 1, &powerSwCounting );
//    }
//    powerSwCounting++;
//    if(powerSwCounting == 3){
//      // <functional>
//      UART_String("reset detect");
//      //P1_2 = 1;
//      powerSwCounting = 0;
//    }
//    else{
//      P0DIR |= BV(1);
//      P0_1 = 1;
//      UART_String("none1");
//    }    
//    osal_nv_write(NV_PW_SW_COUTING_ID, 0, 1, &powerSwCounting);
//  }
//  else
//  {
//    P0DIR |= BV(1);
//    P0_1 = 1;
//    UART_String("none");
//  }
//  // turn on over 1.5s
////  _delay_ms(1500);
////  powerSwCounting = 0;
////  osal_nv_write(NV_PW_SW_COUTING_ID, 0, 1, &powerSwCounting);
//
//}
void zclRGBLedBulb_LeaveNetwork( void )
{
  zclRGBLedBulb_ResetAttributesToDefaultValues();
  
  NLME_LeaveReq_t leaveReq;
  // Set every field to 0
  osal_memset(&leaveReq, 0, sizeof(NLME_LeaveReq_t));

  // This will enable the device to rejoin the network after reset.
  leaveReq.rejoin = FALSE;

  // Set the NV startup option to force a "new" join.
  zgWriteStartupOptions(ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE);

  // Leave the network, and reset afterwards
  if (NLME_LeaveReq(&leaveReq) != ZSuccess) {
    // Couldn't send out leave; prepare to reset anyway
    ZDApp_LeaveReset(FALSE);
  }
}

HAL_ISR_FUNCTION( halInputPort0Isr, P0INT_VECTOR )
{
  HAL_ENTER_ISR();
  UART_String("ISR ne");
  if ( P0IFG & BV(0))
  {
    //halProcessKeyInterrupt();
    osal_stop_timerEx(zclRGBLedBulb_TaskID, RGBLedBulb_RST_COUNTING_EVT);
    powerSwCounting++;
    sprintf(dat,"Count: %d", powerSwCounting);
    UART_String(dat);
    if(powerSwCounting == 3){
      osal_start_timerEx(zclRGBLedBulb_TaskID, RGBLedBulb_EVT_LONG, 100);
    }
    //<>
    osal_start_timerEx(zclRGBLedBulb_TaskID, RGBLedBulb_RST_COUNTING_EVT, 5000);
  }
  /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
  */
  P0IFG = 0;
  P0IF = 0;
  
  CLEAR_SLEEP_MODE();
  HAL_EXIT_ISR();
}
/****************************************************************************
****************************************************************************/


