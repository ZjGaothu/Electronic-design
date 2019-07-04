#include<Wire.h>

uint8_t in_data;

void setup() {
    Wire.begin();
    Serial.begin(9600);
}

void loop() {
    Wire.requestFrom(0x30, 8);
    while(Wire.available() != 8) {
      Serial.print("wait");
      }
    in_data = Wire.read();
    Serial.print(in_data);
    delay(500);
    Wire.requestFrom(0x09, 8);
    while(Wire.available() != 8) {
      Serial.print("wait");}
    in_data = Wire.read();
    Wire.endTransmission();
    Serial.print(in_data);
    delay(500);
}
