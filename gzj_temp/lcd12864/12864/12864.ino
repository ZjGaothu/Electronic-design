/*
LCD  Arduino
PIN1 = GND
PIN2 = 5V
RS(CS) = 8; 
RW(SID)= 9; 
EN(CLK) = 3;
PIN15 PSB = GND;
*/

#include <LCD12864RSPI.h>
#define AR_SIZE(a) sizeof(a)/sizeof(a[0])


unsigned char nochao[]={
  0xC4, 0xE3,
  0xB1, 0xF0,
  0xB3, 0xB3
     };                    //你别吵

void setup()
{
LCDA.Initialise(); // 屏幕初始化
delay(100);
}

void loop()
{
LCDA.CLEAR();//清屏
LCDA.Clearcursor();//清除光标
delay(100);
LCDA.DisplayString(1,2,nochao,AR_SIZE(nochao));//第一行第三格开始
delay(3000);
}
