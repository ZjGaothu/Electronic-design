#include <Wire.h> 
#include"SFE_BMP180.h"          //气压模块
#include "LiquidCrystal_I2C.h"  //引用I2C库
#include"dht11.h"               //温湿模块
#include<SPI.h>                 //SPI
#include<SD.h>                  //SD卡

dht11 DHT11;                   
LiquidCrystal_I2C lcd(0x3F,16,2);  
SFE_BMP180 AirPresure;


#define DHT11PIN 8                // 设置 DHT 引脚 为 Pin 8
#define measurePin  A0                    //pm2.5 
#define ledPower  12  

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

double presureP,presureT;
char presureDelayTime;
            
 
unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;
 
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;
 

void setup()
{
    Serial.begin(9600);
    AirPresure.begin();
    pinMode(ledPower,OUTPUT);
    pinMode(measurePin,INPUT);
}


void loop()
{
    //PM2.5部分
    digitalWrite(ledPower,LOW);
    delayMicroseconds(samplingTime);
 
    voMeasured = analogRead(measurePin);
    
    delayMicroseconds(deltaTime);
    digitalWrite(ledPower,HIGH);
    delayMicroseconds(sleepTime);
    
    calcVoltage = voMeasured*(5.0/1024);
    dustDensity = 0.17*calcVoltage-0.1;
    
    if ( dustDensity < 0)
    {
        dustDensity = 0.00;
    }
    
    Serial.println("Raw Signal Value (0-1023):");
    Serial.println(voMeasured);
    
    Serial.println("Voltage:");
    Serial.println(calcVoltage);
    
    Serial.println("Dust Density:");
    Serial.println(dustDensity);
    delay(20);
    
    //DHT部分
    Serial.print("Humidity (%): ");
    Serial.println((float)DHT11.humidity, 2);
    float hum = (float)DHT11.humidity;
    Serial.print("Temperature °C): ");
    Serial.println((float)DHT11.temperature, 2);
    float tem = (float)DHT11.temperature;

    //BMP部分
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
