#include <Wire.h> 
#include"dht11.h"
#include "LiquidCrystal_I2C.h" //引用I2C库
dht11 DHT11;
LiquidCrystal_I2C lcd(0x3F,16,2);  

// 设置 DHT 引脚 为 Pin 8
#define DHT11PIN 8
#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif
uint8_t heart[8] = {B00000,B11110,B11010,B01001,B11110,B11010,B01001};

void setup(){
  Serial.begin(9600);
  Serial.println();
  lcd.createChar(0,heart);
  while(Serial.read() >= 0) {} // clear serial port when begin
  lcd.init();                  // 初始化LCD
  lcd.backlight();             //设置LCD背景等亮
  lcd.home();
}


void loop() {
  Serial.println("\n");

  int chk = DHT11.read(DHT11PIN);

  // 测试 DHT 是否正确连接
  Serial.print("Read sensor: ");
  switch (chk)
  {
    case DHTLIB_OK: 
    Serial.println("OK"); 
    break;
    case DHTLIB_ERROR_CHECKSUM: 
    Serial.println("Checksum error"); 
    break;
    case DHTLIB_ERROR_TIMEOUT: 
    Serial.println("Time out error"); 
    break;
    default: 
    Serial.println("Unknown error"); 
    break;
  }

  // 获取测量数据
  Serial.print("Humidity (%): ");
  Serial.println((float)DHT11.humidity, 2);
  float hum = (float)DHT11.humidity;
  lcd.setCursor(0, 0);
  lcd.print("Hum : ");
  lcd.print(hum);
  lcd.print(" %");
  Serial.print("Temperature °C): ");
  Serial.println((float)DHT11.temperature, 2);
  float tem = (float)DHT11.temperature;
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(tem);
  lcd.print((char)223);
  lcd.print("C ");
  


  delay(1000);
}


//引脚连接
//VDD 供电为 3.3 ∼ 5V DC，2.DATA 串行数据，单总线 3.NC 空脚
//4.GND 接地，电源负极
//data和供电端接上拉电阻 5k左右即可
