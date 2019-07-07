//TODO change dht11 library
#include <Wire.h> //I2C device library
#include "SFE_BMP180.h"
#include "dht11.h"
#include "LiquidCrystal_I2C.h"
#include <IRremote.h>
#include <Servo.h>
#include <SPI.h>
#include <SD.h>

//PIN define TODO maybe use a .h file
#define T_H_PIN 8 // Temperature and humidity sensor dht11
#define DUST_PIN A0 // dust sensor GP2Y1050AU0F
#define DUST_LED_PIN 12 // DUST LED, flashing to check light to get dust density
#define LIGHT_PULSE_PIN 13
#define IR_PIN 9 // IR remote receiver pin
#define SERVO_PIN 5 // TODO check, PWM need
#define CHAOS_PIN A1

// I2C use hard I2C in arduino A4 - SDA, A5 - SCK
// I2C device, define address
#define LCD_ADDR 0x3f
#define KEY_RADAR_ADDR 0X20 // PCF8574, GPIO to I2C as a serial IO
#define AIR_PRESURE_ADDR 0x77 // SFE_BMP180

dht11 t_h_sensor;
SFE_BMP180 air_presure_sensor;
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
IRrecv irrecv(IR_PIN);
Servo radar_sweeper;
File record_file;

// TODO unknow para
const unsigned int samplingTime PROGMEM = 280;
const unsigned int deltaTime PROGMEM = 40;
const unsigned int sleepTime PROGMEM = 9680;
const unsigned int show_refresh_time PROGMEM = 700; // ms

// measure storeage
float dustVal = 0;
double presureP,presureT; // TODO maybe cancel 1 variable
uint8_t key_radar_code; //temp P7-P4 for key, P3 for radar TODO check it
int light_pulse_time = 0;
decode_results ir_code;

// Byte set as follow, write 
/*
P7 - P0
roll show xxxx xxx0
stop roll xxxx xxx1
in stop roll bit P6 - P4, P3 - P1, the two value to show
000 temperature
001 humidity
010 pressure
011 dust density(PM2.5)
100 light intensity 
101 animal detect(force)
*/

uint8_t state; 

// btn set
// up down start/stop_roll sensitivity_set

// use servo to Sweep Radar, check if there is animal
void SweepRadar();
void Btn2State();
void PrintLCDByState();
void DecodeAndPrint(uint8_t print_code);
void IR2State();

void setup() {
    Serial.begin(9600); // serial to bluetooth by module HC-06 TODO debug use, maybe end
    Wire.begin(); // set I2C communicate
    air_presure_sensor.begin();
    irrecv.enableIRIn();
    radar_sweeper.attach(SERVO_PIN);
    lcd.init();
    lcd.backlight();
    pinMode(DUST_LED_PIN, OUTPUT);
    pinMode(DUST_PIN, INPUT);
    pinMode(LIGHT_PULSE_PIN, INPUT);
    pinMode(CHAOS_PIN, INPUT);
    state = 0x00;
}

void loop() {
    // Temperature and humidity
    t_h_sensor.read(T_H_PIN); // send request to Temperature and humidity sensor, without check sum. // data in t_h_sensor.humidity_int .humidity_dec, .temperature_int, .temperature_dec
    delay(20); // TODO cancel it?

    // air pressure, without error check
    air_presure_sensor.getPressure(presureP, presureT);
    delay(20);

    // PM2.5
    digitalWrite(DUST_LED_PIN, LOW);
    delayMicroseconds(samplingTime);

    dustVal = ((analogRead(DUST_PIN) * (5.0 / 1024) * 0.17 - 0.1) / 1024 - 0.0356) * 120000 * 0.035; // TODO check formula
    dustVal = dustVal > 0 ? dustVal : 0;

    delayMicroseconds(deltaTime);
    digitalWrite(DUST_LED_PIN, HIGH);

    delay(20); // TODO cancel it?

    // light TODO add formula and cancel time data
    light_pulse_time = pulseIn(LIGHT_PULSE_PIN, LOW);
    delay(20); // TODO cancel it
    
    SweepRadar();

    if (Wire.requestFrom((uint8_t)KEY_RADAR_ADDR, (uint8_t)1) != 1) { // request 1 byte from key radar serial
        key_radar_code = 0; // for error
    } else {
        key_radar_code = Wire.read(); // get both radar for animal data and key data
    }
    // TODO now code order use key_radar_code & 0x08 == 1 for is animal

    // btn read to change state
    Btn2State();
    // ir_remote check TODO
    IR2State();
    // show
    PrintLCDByState();
}

// use servo to Sweep Radar, check if there is animal
// TODO inline or marco?
void SweepRadar() {
    radar_sweeper.write(analogRead(CHAOS_PIN) * 180 / 1023); // turn it to 0-180 TODO change formula?
    delayMicroseconds(100); // TODO check time
    radar_sweeper.write(analogRead(CHAOS_PIN) * 180 / 1023); // turn it to 0-180 TODO change formula?
}

void Btn2State() {
    // TODO maybe simplify bool operation
    if ((state & 0x01 == 0) && (key_radar_code & 0x20 != 0)) {
        state = 0x03; // stop roll and show temperature and humidity
        return;
    }
    if ((state & 0x01 != 0) && (key_radar_code & 0x80 != 0)) { // up
        if (state & 0x70 == 0) { // the first line show temperature(code 0) now, set for circle to animal detect(code 5)
            state += 0x60;
        }
        if (state & 0x0e == 0) { // the second line show temperature(code 0) now, set for circle to animal detect(code 5)
            state += 0x0c;
        }
        state -= 0x12; // up line, both 3 bits show code -1
        return;
    }
    if ((state & 0x01 != 0) && (key_radar_code & 0x80 != 0)) { // down
        state += 0x12;
        if (state & 0x70 == 0x60) { // the first line show animal detect(code 5) before, set now for circle to temperature(code 0)
            state &= 0x0f;
        }
        if (state & 0x0e == 0x0c) { // the second line show animal detect(code 5) before, set now for circle to animal detect
            state &= 0xf1;
        }
         // up line, both 3 bits show code +1
         return;
    }
    //TODO sensitivity_set
    return;
}

void PrintLCDByState() {
    if (state & 0x01 == 1) { // stop roll
        lcd.clear();
        lcd.setCursor(0, 0);
        DecodeAndPrint((state & 0x70) >> 4); // show the first line
        lcd.setCursor(0, 1);
        DecodeAndPrint((state & 0x0e) >> 1); //show the second line
        return;
    } else { // roll TODO roll time?
        // temperature and humidity
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Temp:"));
        lcd.print(t_h_sensor.temperature_int);
        lcd.print(F("."));
        lcd.print(t_h_sensor.temperature_dec);
        lcd.print((char)223); // circle for centigrade
        lcd.print(F("C "));
        lcd.setCursor(0, 1);
        lcd.print(F("Hum: "));
        lcd.print(t_h_sensor.humidity_int);
        lcd.print(F("."));
        lcd.print(t_h_sensor.humidity_dec);
        delay(show_refresh_time);

        // pressure and PM2.5
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("AirP:"));
        lcd.print(presureP);
        lcd.print(F("bar"));
        lcd.setCursor(0, 1);
        lcd.print(F("PM2.5:"));
        lcd.print(dustVal);
        delay(show_refresh_time);

        //light and animal detect
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("I:   "));
        lcd.print(light_pulse_time); // TODO change show
        lcd.print(F("lx"));
        lcd.setCursor(0, 1);
        lcd.print(F("isForce:"));
        lcd.print(key_radar_code & 0x08);
        //TODO delay ?
        return;
    }
}

void DecodeAndPrint(uint8_t code) {
    switch (code) {
        case 0x00: // temperature
            lcd.print(F("Temp:"));
            lcd.print(t_h_sensor.temperature_int);
            lcd.print(F("."));
            lcd.print(t_h_sensor.temperature_dec);
            lcd.print((char)223); // circle for centigrade
            lcd.print(F("C "));
            return;
        case 0x01: // humidity
            lcd.print(F("Hum: "));
            lcd.print(t_h_sensor.humidity_int);
            lcd.print(F("."));
            lcd.print(t_h_sensor.humidity_dec);
            return;
        case 0x02: // Air pressure
            lcd.print(F("AirP:"));
            lcd.print(presureP);
            lcd.print(F("bar"));
            return;
        case 0x03: // PM2.5
            lcd.print(F("PM2.5:"));
            lcd.print(dustVal);
            return;
        case 0x04: // light
            lcd.print(F("I:   "));
            lcd.print(light_pulse_time); // TODO change show
            lcd.print(F("lx"));
            return;
        case 0x05: // animal detect
            lcd.print(F("isForce:"));
            lcd.print(key_radar_code & 0x08);
            return;
        default:
            return;
        }
}

void IR2State() {

    return;
}