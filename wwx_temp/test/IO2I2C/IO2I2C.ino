#include<Wire.h>

#define PCF8574_ADDR 0x20 // TODO maybe change it
uint8_t in_data;

void setup() {
    Wire.begin();
    Serial.begin(9600);
}

void loop() {
    if (Wire.requestFrom(0x20, (uint8_t)1) != 1) { // length 1 means 1 Byte, 1Byte will organize(H2L) P7_P0 in serial, port in air or write port will read as high
      in_data = 0;
    } else {
      in_data = Wire.read();
    }
    Serial.println(in_data);
    delay(500);
}
