#include"SFE_BMP180.h"

SFE_BMP180 AirPresure;
char presureDelayTime;
double presureP, presureT;

void setup() {
  Serial.begin(9600);
  AirPresure.begin();
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

      //换算成标准大气压
      Serial.print("换算标准大气压：");
      Serial.print(presureP / 1000.0);
      Serial.println(" atm");
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
