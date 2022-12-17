#define RED     1
#define GREEN   2
#define BLUE    3


#include "ZComDef.h"
#include "onboard.h"
#include "bitmasks.h"
#include "uart.h"


void PWM_Init(void);
uint8 setDutyCyclePWM(uint8 channel, uint8 duty); // duty 0-255