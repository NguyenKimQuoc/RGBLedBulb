#define SECURE 1
#define TC_LINKKEY_JOIN
#define NV_INIT
#define NV_RESTORE
#define xZTOOL_P1
#define MT_TASK
#define MT_APP_FUNC
#define MT_SYS_FUNC
#define MT_ZDO_FUNC
#define MT_ZDO_MGMT
#define MT_APP_CNF_FUNC
//#define LEGACY_LCD_DEBUG
//#define LCD_SUPPORTED DEBUG
#define MULTICAST_ENABLED FALSE
#define ZCL_READ
#define ZCL_WRITE
#define ZCL_BASIC
#define ZCL_IDENTIFY
#define ZCL_SCENES
#define ZCL_GROUPS
#define ZCL_ON_OFF
#define ZCL_REPORTING_DEVICE

#define DISABLE_GREENPOWER_BASIC_PROXY
#define DEFAULT_CHANLIST 0x07FFF800  // ????? ??? ?????? ?? ???? ???????

#define HAL_SONOFF // ??????? ?????? ??? Sonoff Zigbee
#define UART_DEBUG
//#define DO_DEBUG_UART
#define ZLL_HW_LED_LAMP
#define ZCL_LIGHT_LINK_ENHANCE
#define ZCL_LEVEL_CTRL
#define ZCL_COLOR_CTRL
#define PWM_ALT2
#define THERMAL_SHUTDOWN
#define ZLL_1_0_HUB_COMPATIBILITY

#include "hal_board_cfg_RGBLedBulb.h"

#define SET(PIN,N) (PIN |=  (1<<N))
#define CLR(PIN,N) (PIN &= ~(1<<N))