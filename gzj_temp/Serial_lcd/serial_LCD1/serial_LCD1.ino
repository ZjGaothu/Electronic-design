#include <Wire.h> 
#include "LiquidCrystal_I2C.h" //引用I2C库
#define MAXLENGTH 32
LiquidCrystal_I2C lcd(0x3F,16,2);  


char comchar; // char by char serial

int numdata = 0;
char buffer[MAXLENGTH]; // chars serial to LCD

void setup() {
   Serial.begin(9600);
   while(Serial.read() >= 0) {} // clear serial port when begin
   lcd.init();                  // 初始化LCD
   lcd.backlight();             //设置LCD背景等亮
}

void loop() {
   if(Serial.available() > 0){   //串口缓冲区收到数据
      delay(100); //TODO may change, wait a string send finish
      numdata = Serial.readBytes(buffer, MAXLENGTH);
      Serial.print("Serial readBytes: ");
      Serial.println(buffer);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(buffer);
      lcd.setCursor(0, 1);
      lcd.print(buffer + 16);
      delay(1000);
   }

   // clear buffer
   while(Serial.read() >= 0){}
   for(int i = 0; i < MAXLENGTH; i++) {
      buffer[i] = '\0';
   }
}
 