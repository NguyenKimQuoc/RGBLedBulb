#include "pwm.h"
uint16 period;

void PWM_Init(){
  //PERCFG |= 0x03;
  //P2DIR = (P2DIR & ~0xC0) | 0x80; // Give priority to Timer 1
  PERCFG |= b01000000;
//  P2SEL |= BV(3);
  //P2SEL |= b00010000; 
  //P1SEL |= BV(1)|BV(0);  // Set P1_1 to peripheral
//  P1DIR |= BV(1)|BV(0);
//  P0SEL |= BV(6)|BV(5)|BV(4);
//  P0DIR |= BV(7)|BV(6);
  
  T1CC0L = 0xFF;   // PWM signal period
  T1CC0H = 0xFF;
  
  T1CC1L = 0xFF;  // PWM duty cycle
  T1CC1H = 0xFF;
  T1CCTL1 = 0x1c;
  
  T1CC2L = 0xFF;  // PWM duty cycle
  T1CC2H = 0xFF;
  T1CCTL2 = 0x1c;
  
  T1CC3L = 0xFF;  // PWM duty cycle
  T1CC3H = 0xFF;
  T1CCTL3 = 0x1c;
  
  T1CTL |= b00000011; // divide with 128 and to do i up-down mode
  period = (T1CC0H << 8);
  period |= (T1CC0L & 0xFF);
  
}

uint8 setDutyCyclePWM(uint8 channel, uint8 duty){
  uint8 dutyFilter = (duty == 255)?0:(duty == 0)?255:duty;
  uint16 duty16 = (uint16) (period*((double)dutyFilter)/((double)255));
  switch (channel){
  case RED:
    T1CC1L = (uint8) (duty16 & 0xFF);
    T1CC1H = (uint8) ((duty16 >> 8) & 0xFF);
    break;
  case GREEN:
    T1CC2L = (uint8) (duty16 & 0xFF);
    T1CC2H = (uint8) ((duty16 >> 8) & 0xFF);
    break;
  case BLUE:
    T1CC3L = (uint8) (duty16 & 0xFF);
    T1CC3H = (uint8) ((duty16 >> 8) & 0xFF);
    break;
  default:
    return 0;
    break;
  }
  return 1;
}

