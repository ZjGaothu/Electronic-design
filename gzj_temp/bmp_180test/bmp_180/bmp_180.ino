#include"SFE_BMP180.h"
#include <Wire.h> 
#include "LiquidCrystal_I2C.h" //引用I2C库
SFE_BMP180 AirPresure;

LiquidCrystal_I2C lcd(0x3F,16,2);

char presureDelayTime;
double presureP, presureT;

void setup() {
  Serial.begin(9600);
  AirPresure.begin();
  lcd.init();                  // 初始化LCD
  lcd.backlight();             //设置LCD背景等亮
}

void loop()
{
  presureDelayTime = AirPresure.startPressure(3);
  if (presureDelayTime != 0)
  {
    delay(presureDelayTime);
    presureDelayTime = AirPresure.getPressure(presureP, presureT);
    if (presureDelayTime != 0)
    {
      //当前气压
      Serial.print("当前气压是: ");
      Serial.print(presureP);
      Serial.println(" bar");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(presureP);

      //换算成标准大气压
      Serial.print("换算标准大气压：");
      Serial.print(presureP / 1000.0);
      Serial.println(" atm");
      lcd.setCursor(0, 1);
      lcd.print(presureP / 1000.0);
    }
    else
    {
      Serial.println("ERROR");
    }
  }
  else
  {
    Serial.println("ERROR");
  }
  delay(1000);
}

//引脚连接 
//5V—VIN
//GND–GND
//A5—SCL
//A4—SDA
//当前实现和LCD1602联调，库内实现了I2C的地址管理
