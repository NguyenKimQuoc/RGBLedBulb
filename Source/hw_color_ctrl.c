#include "hw_color_ctrl.h"
#include "hueToXyTable.c"
#include "math.h"
#define DISTANCE(a,b)  (((a) > (b)) ? ((a) - (b)) : ((b)-(a)))

static void hwLight_UpdateLampColor( uint16 colorX, uint16 colorY, uint8 level);
static void hwLight_Convert_xyY_to_RGB(float x, float y, float Y, uint16 *R, uint16 *G,uint16 *B);
static void hwLight_GammaCorrectRGB(float *R, float *G, float *B);
static uint8 hwLight_XyToSat(uint16 x, uint16 y, uint8 hue);

static int32 zclRGBLedBulb_StepSaturation_256 = 0;
static uint16 zclRGBLedBulb_SaturationRemainingTime = 0;
static uint16 zclRGBLedBulb_CurrentSaturation_256 = 0;

static int32 zclRGBLedBulb_StepHue_256 = 0;
static uint16 zclRGBLedBulb_HueRemainingTime = 0;
static uint16 zclRGBLedBulb_CurrentHue_256 = 0;


static int32 zclRGBLedBulb_StepColorX_256 = 0;
static int32 zclRGBLedBulb_StepColorY_256 = 0;
static uint32 zclRGBLedBulb_CurrentX_256 = 0;
static uint32 zclRGBLedBulb_CurrentY_256 = 0;

static byte zclLight_TaskID;

void zclRGBLedBulb_init( byte taskID )
{
  zclLight_TaskID = taskID;

  //Move to default color
  zclRGBLedBulb_HueRemainingTime = 0;
  hwLight_ApplyUpdate( &zclRGBLedBulb_CurrentHue,
                       &zclRGBLedBulb_CurrentHue_256,
                       &zclRGBLedBulb_StepHue_256,
                       &zclRGBLedBulb_HueRemainingTime,
                       COLOR_HUE_MIN, COLOR_HUE_MAX, TRUE );
  zclRGBLedBulb_SaturationRemainingTime = 0;
  hwLight_ApplyUpdate( &zclRGBLedBulb_CurrentSaturation,
                       &zclRGBLedBulb_CurrentSaturation_256,
                       &zclRGBLedBulb_StepSaturation_256,
                       &zclRGBLedBulb_SaturationRemainingTime,
                       COLOR_SAT_MIN, COLOR_SAT_MAX, FALSE );
}


void hwLight_Convert_xyY_to_RGB(float x, float y, float LinearY, uint16 *R, uint16 *G, uint16 *B)
{
  float Y;
  float X;
  float Z;
  float f_R;
  float f_G;
  float f_B;

  //gamma correct the level
  Y = pow( ( LinearY / LEVEL_MAX ), (float)GAMMA_VALUE ) * (float)LEVEL_MAX;


  // from xyY to XYZ
  if (y != 0)
  {
    do
    {
      X = x * ( Y / y );
      Z = ( 1 - x - y ) * ( Y / y );
    }
    while( ((X > 95.047) || (Z > 108.883)) && ((uint8)(Y--) > 0));
    // normalize variables:
    // X from 0 to 0.95047
    X = (X > 95.047 ? 95.047 : (X < 0 ? 0 : X));
    X = X / 100;
    // Z from 0 to 1.08883
    Z = (Z > 108.883 ? 108.883 : (Z < 0 ? 0 : Z));
    Z = Z / 100;
  }
  else
  {
    X = 0;
    Z = 0;
  }
  // Y from 0 to 1
  Y = (Y > 100 ? 100 : (Y < 0 ? 0 : Y));
  Y = Y / 100;

  // transformation according to standard illuminant D65.
  f_R = X *  3.2406 + Y * -1.5372 + Z * -0.4986;
  f_G = X * -0.9689 + Y *  1.8758 + Z *  0.0415;
  f_B = X *  0.0557 + Y * -0.2040 + Z *  1.0570;

  //color correction
  hwLight_GammaCorrectRGB(&f_R, &f_G, &f_B);


  // truncate results exceeding 0..1
  f_R = (f_R > 1.0 ? 1.0 : (f_R < 0 ? 0 : f_R));
  f_G = (f_G > 1.0 ? 1.0 : (f_G < 0 ? 0 : f_G));
  f_B = (f_B > 1.0 ? 1.0 : (f_B < 0 ? 0 : f_B));

  *R = (uint16)(f_R * PWM_FULL_DUTY_CYCLE);
  *G = (uint16)(f_G * PWM_FULL_DUTY_CYCLE);
  *B = (uint16)(f_B * PWM_FULL_DUTY_CYCLE);
}

void hwLight_GammaCorrectRGB(float *R, float *G, float *B)
{
  if ( *R > 0.003 ) *R = (1.22 * ( pow(*R, ( 1/1.5 ) )) - 0.040);
  else                      *R = 0; //1.8023 * var_R;
  if ( *G > 0.003) *G = (1.22 * ( pow(*G, ( 1/1.5 ) )) - 0.040);
  else                     *G = 0; //1.8023 * var_G;
  if ( *B > 0.003 ) *B = (1.09 * ( pow(*B, ( 1/1.5 ) )) - 0.050);
  else                     *B = 0; //1.8023 * var_B;
}

ZStatus_t zclRGBLedBulb_MoveToColorCB( zclCCMoveToColor_t *pCmd )
{
  hwLight_UpdateColorMode(COLOR_MODE_CURRENT_X_Y); 
  zclRGBLedBulb_CurrentX_256 = ((int32)zclRGBLedBulb_CurrentX)<<8;
  zclRGBLedBulb_CurrentY_256 = ((int32)zclRGBLedBulb_CurrentY)<<8;
  zclRGBLedBulb_ColorMode=COLOR_MODE_CURRENT_X_Y;
  zclRGBLedBulb_EnhancedColorMode=COLOR_MODE_CURRENT_X_Y;

  //if transition time = 0 then do immediately
  if ( pCmd->transitionTime == 0 )
  {
      zclRGBLedBulb_ColorRemainingTime = 1;
  }
  else
  {
    zclRGBLedBulb_ColorRemainingTime = pCmd->transitionTime;
  }

  zclRGBLedBulb_StepColorX_256 = ((int32)pCmd->colorX - zclRGBLedBulb_CurrentX)<<8;
  zclRGBLedBulb_StepColorX_256 /= (int32)zclRGBLedBulb_ColorRemainingTime;
  zclRGBLedBulb_StepColorY_256 = ((int32)pCmd->colorY - zclRGBLedBulb_CurrentY)<<8;
  zclRGBLedBulb_StepColorY_256 /= (int32)zclRGBLedBulb_ColorRemainingTime;

  hwLight_ApplyUpdate16b( &zclRGBLedBulb_CurrentX,
                          &zclRGBLedBulb_CurrentX_256,
                          &zclRGBLedBulb_StepColorX_256,
                          &zclRGBLedBulb_ColorRemainingTime,
                          COLOR_XY_MIN, COLOR_XY_MAX, FALSE );
  if ( zclRGBLedBulb_ColorRemainingTime != 0xFFFF )
  {
    zclRGBLedBulb_ColorRemainingTime++;
  }
  hwLight_ApplyUpdate16b( &zclRGBLedBulb_CurrentY,
                          &zclRGBLedBulb_CurrentY_256,
                          &zclRGBLedBulb_StepColorY_256,
                          &zclRGBLedBulb_ColorRemainingTime,
                          COLOR_XY_MIN, COLOR_XY_MAX, FALSE );

  if ( zclRGBLedBulb_ColorRemainingTime )
  {
    //set a timer to make the change over 100ms
    osal_start_timerEx( zclLight_TaskID, COLOR_PROCESS_EVT , 100 );
  }

  return ( ZSuccess );
}

static void hwLight_satToXy(uint16 *x, uint16 *y, uint8 sat)
{
  float xDelta, yDelta;
  uint16 localSat;

  localSat = 255 - sat;

  xDelta = (int32)*x - WHITE_POINT_X;
  yDelta = (int32)*y - WHITE_POINT_Y;

  xDelta = xDelta * (float)localSat/0xFF;
  yDelta = yDelta * (float)localSat/0xFF;

  *x = (uint16) (*x - xDelta);
  *y = (uint16) (*y - yDelta);

  return;
}
void hwLight_ApplyUpdate( uint8 *pCurrentVal, uint16 *pCurrentVal_256, int32 *pStepVal_256, uint16 *pRemainingTime, uint8 minLevel, uint8 maxLevel, bool wrap )
{
  if( (*pStepVal_256 > 0) && ((((int32)*pCurrentVal_256 + *pStepVal_256)/256) > maxLevel) )
  {
    if(wrap)
    {
      *pCurrentVal_256 = (uint16)minLevel*256 + ( ( *pCurrentVal_256 + *pStepVal_256 ) - (uint16)maxLevel*256 ) - 256;
    }
    else
    {
      *pCurrentVal_256 = (uint16)maxLevel*256;
    }
  }
  else if( (*pStepVal_256 < 0) && ((((int32)*pCurrentVal_256 + *pStepVal_256)/256) < minLevel) )
  {
    if(wrap)
    {
      *pCurrentVal_256 = (uint16)maxLevel*256 - ( (uint16)minLevel*256 - ((int32)*pCurrentVal_256 + *pStepVal_256) ) + 256 ;
    }
    else
    {
      *pCurrentVal_256 = (uint16)minLevel*256;
    }
  }
  else
  {
    *pCurrentVal_256 += *pStepVal_256;
  }

  if (*pStepVal_256 > 0)
  {
     //fraction step compensation
    *pCurrentVal = ( *pCurrentVal_256 + 127 ) / 256;
  }
  else
  {
    *pCurrentVal = ( *pCurrentVal_256 / 256 );
  }

  if (*pRemainingTime == 0x0)
  {
    // align variables
    *pCurrentVal_256 = ((uint16)*pCurrentVal)*256 ;
    *pStepVal_256 = 0;
  }
  else if (*pRemainingTime != 0xFFFF)
  {
    *pRemainingTime = *pRemainingTime-1;
  }

  hwLight_UpdateLampColor(zclRGBLedBulb_CurrentX, zclRGBLedBulb_CurrentY, zclLevel_CurrentLevel);
}
void hwLight_ApplyUpdate16b( uint16 *pCurrentVal, uint32 *pCurrentVal_256, int32 *pStepVal_256, uint16 *pRemainingTime, uint16 minLevel, uint16 maxLevel, bool wrap )
{
  if( (*pStepVal_256 > 0) && ((((int32)*pCurrentVal_256 + *pStepVal_256) / 256) > maxLevel) )
  {
    if(wrap)
    {
      *pCurrentVal_256 = ( (uint32)minLevel * 256 ) + ( ( *pCurrentVal_256 + *pStepVal_256 ) - ( (uint32)maxLevel * 256 ) ) - 256;
    }
    else
    {
      *pCurrentVal_256 = (uint32)maxLevel * 256;
    }
  }
  else if( (*pStepVal_256 < 0) && ((((int32)*pCurrentVal_256 + *pStepVal_256) / 256 ) < minLevel) )
  {
    if(wrap)
    {
      *pCurrentVal_256 = ( (uint32)maxLevel * 256 ) - ( ( (uint32)minLevel * 256 ) - ((int32)*pCurrentVal_256 + *pStepVal_256) ) + 256;
    }
    else
    {
      *pCurrentVal_256 = (uint32)minLevel * 256;
    }
  }
  else
  {
    *pCurrentVal_256 += *pStepVal_256;
  }

  if (*pStepVal_256 > 0)
  {
    //fraction step compensation
    *pCurrentVal = ( *pCurrentVal_256 + 127 ) / 256;
  }
  else
  {
    *pCurrentVal = ( *pCurrentVal_256 / 256 );
  }

  if (*pRemainingTime == 0x0)
  {
    // align variables
    *pCurrentVal_256 = ((uint32)*pCurrentVal) * 256;
    *pStepVal_256 = 0;
  }
  else if (*pRemainingTime != 0xFFFF)
  {
    *pRemainingTime = *pRemainingTime-1;
  }

  //hwLight_Refresh( REFRESH_AUTO );
  if(zclRGBLedBulb_ColorMode == COLOR_MODE_CURRENT_X_Y)
  {
    hwLight_UpdateColorMode(COLOR_MODE_CURRENT_HUE_SATURATION);
  }
  else
  {
    hwLight_UpdateColorMode(COLOR_MODE_CURRENT_X_Y);
  }
  hwLight_UpdateLampColor(zclRGBLedBulb_CurrentX, zclRGBLedBulb_CurrentY, zclLevel_CurrentLevel);
  
}

void hwLight_UpdateColorMode(uint8 NewColorMode)
{
  uint8 idx, chosenIdx=0;
  uint32 currDist=0, minDist = 0xFFFFF;

  if( NewColorMode != zclRGBLedBulb_ColorMode )
  {
    if( NewColorMode == COLOR_MODE_CURRENT_X_Y )
    {
      //update the current xy values from hue and sat
      zclRGBLedBulb_CurrentX = hueToX[zclRGBLedBulb_CurrentHue];
      zclRGBLedBulb_CurrentY = hueToY[zclRGBLedBulb_CurrentHue];

      hwLight_satToXy( &zclRGBLedBulb_CurrentX, &zclRGBLedBulb_CurrentY, zclRGBLedBulb_CurrentSaturation );
    }
    else if( NewColorMode == COLOR_MODE_CURRENT_HUE_SATURATION )
    {
      //update the current hue/sat values from xy
      //Loop thrugh hueToX/Y tables and look for the value closest to the
      //zclRGBLedBulb_CurrentX and zclRGBLedBulb_CurrentY
      for( idx = 0; idx != 0xFF; idx++ )
      {
        currDist = ( (uint32)DISTANCE(hueToX[idx], zclRGBLedBulb_CurrentX)
                     + DISTANCE(hueToY[idx], zclRGBLedBulb_CurrentY) );

        if ( currDist < minDist )
        {
          chosenIdx = idx;
          minDist = currDist;
        }
      }
      zclRGBLedBulb_CurrentHue = chosenIdx;

      zclRGBLedBulb_EnhancedCurrentHue = (uint16)zclRGBLedBulb_CurrentHue << 8;

      zclRGBLedBulb_CurrentSaturation = hwLight_XyToSat( zclRGBLedBulb_CurrentX, zclRGBLedBulb_CurrentY, zclRGBLedBulb_CurrentHue );
    }
    else //do nothing COLOR_MODE_COLOR_TEMPERATURE not supported
    {
    }
  }

  return;
}

static void hwLight_UpdateLampColor( uint16 colorX, uint16 colorY, uint8 level)
{
  uint16 redP, greenP, blueP;
  char buffer[100];

  hwLight_Convert_xyY_to_RGB((float)colorX/0xFFFF, (float)colorY/0xFFFF, (float) level*(100/(float)0xFF),
                     &redP, &greenP, &blueP);

  sprintf(buffer, "Light-CMD: r: %d // g: %d // b: %d\n", redP, greenP, blueP);
  UART_String(buffer); 
  setDutyCyclePWM (RED,   redP);
  setDutyCyclePWM (GREEN, greenP );
  setDutyCyclePWM (BLUE,  blueP);
}

void zclRGBLedBulb_process( uint16 *events )
{
  if ( *events & COLOR_PROCESS_EVT )
  {
    if(zclRGBLedBulb_EnhancedColorMode == COLOR_MODE_CURRENT_X_Y)
    {
      if(zclRGBLedBulb_ColorRemainingTime)
      {
          hwLight_ApplyUpdate16b( &zclRGBLedBulb_CurrentX,
                                  &zclRGBLedBulb_CurrentX_256,
                                  &zclRGBLedBulb_StepColorX_256,
                                  &zclRGBLedBulb_ColorRemainingTime,
                                  COLOR_XY_MIN, COLOR_XY_MAX, FALSE );
          if(zclRGBLedBulb_ColorRemainingTime != 0xFFFF)
          {
            zclRGBLedBulb_ColorRemainingTime++; //hwLight_ApplyUpdate16b decrements each time.
          }
          hwLight_ApplyUpdate16b( &zclRGBLedBulb_CurrentY,
                                  &zclRGBLedBulb_CurrentY_256,
                                  &zclRGBLedBulb_StepColorY_256,
                                  &zclRGBLedBulb_ColorRemainingTime,
                                  COLOR_XY_MIN, COLOR_XY_MAX, FALSE );
      }
    }

    if ( zclRGBLedBulb_ColorRemainingTime )
    {
      //set a timer to make the change over 100ms
      osal_start_timerEx( zclLight_TaskID, COLOR_PROCESS_EVT, 100 );
    }

    *events = *events ^ COLOR_PROCESS_EVT;
  }

  return;
}

static uint8 hwLight_XyToSat(uint16 x, uint16 y, uint8 hue)
{
  uint32 xyDeltas, hueDeltas;

  //we try to avoid polor to cartesan conversion using proportional scaling
  xyDeltas = ((uint32)DISTANCE(x, WHITE_POINT_X) + DISTANCE(y, WHITE_POINT_Y));
  hueDeltas = ((uint32)DISTANCE(hueToX[hue], WHITE_POINT_X) + DISTANCE(hueToY[hue], WHITE_POINT_Y));

  return ( (hueDeltas) ? ((xyDeltas * COLOR_SAT_MAX) / hueDeltas) : COLOR_SAT_MAX );
}