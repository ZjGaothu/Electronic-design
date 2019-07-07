// pin configuration
/*
dht11 - 8 temperature and humidity sensor
GP2Y1050AU0F signal - A0(remember filter); led - 12
light_pulse - 13
IR - 9
servo - 5
chaos - A1
SD MOSI - 11; MISO - 12; SCK - 13; CS - 4
I2C device
A4 - SDA, A5 - SCK
*/
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
#define SD_CS_PIN 4
#define REFRESH_INTR_PIN 3

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
int light_pulse_time = 0;
decode_results ir_code;

uint8_t key_radar_code; //temp P7-P4 for key, P3 for radar TODO check it
// key setting
/*
P7 - P4
up down start/stop_roll set_sensitivity
*/

// IR remote controller code
/*
    CH-     CH      CH+
    FFA25D  FF629D  FFE21D
    |<<     >>|     >||
    FF22DD  FF02FD  FFC23D
    -       +       EQ
    FFE01F  FFA857  FF906F
    0       100+    200+
    FF6897  FF9867  FFB04F
    1       2       3
    FF30CF  FF18E7  FF7A85
    4       5       6
    FF10EF  FF38C7  FF5AA5
    7       8       9
    FF42BD  FF4AB5  FF52AD
*/
#define UP_IR 0xffa857
#define DOWN_IR 0xffe01f
#define ROLL_IR 0xff906f
#define K0_IR 0xff6897
#define K1_IR 0xff30cf
#define K2_IR 0xff9867
#define K3_IR 0xff7a85
#define K4_IR 0xff10ef
#define K5_IR 0xff38c7

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
void RrfreshMeasure(); // TODO attahc interupt
void SweepRadar();
void Btn2State();
void PrintLCDByState();
void DecodeAndPrint(uint8_t print_code);
void IR2State();
uint8_t IRNum2Num(const decode_results& ir_num);
void BluetoothSend();
void FileWrite();

void setup() {
    Serial.begin(9600); // serial to bluetooth by module HC-06 TODO debug use, maybe end
    Wire.begin(); // set I2C communicate
    air_presure_sensor.begin();
    irrecv.enableIRIn();
    radar_sweeper.attach(SERVO_PIN);
    if (!SD.begin(SD_CS_PIN)) {
        Serial.print(F("no SD card"));
    }
    lcd.init();
    lcd.backlight();
    pinMode(DUST_LED_PIN, OUTPUT);
    pinMode(DUST_PIN, INPUT);
    pinMode(LIGHT_PULSE_PIN, INPUT);
    pinMode(CHAOS_PIN, INPUT);
    state = 0x00;
    // TODO interrupts refresh
    // pinMode(REFRESH_INTR_PIN, INPUT);
    // attachInterrupt(digitalPinToInterrupt(REFRESH_INTR_PIN), RrfreshMeasure, RISING);
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

    // bluetooth send
    BluetoothSend();
    // write in SD card
    FileWrite();
}

void RrfreshMeasure() {
    // Temperature and humidity
    t_h_sensor.read(T_H_PIN); // send request to Temperature and humidity sensor, without check sum. // data in t_h_sensor.humidity_int .humidity_dec, .temperature_int, .temperature_dec
    delayMicroseconds(20000); // TODO cancel it?

    // air pressure, without error check
    air_presure_sensor.getPressure(presureP, presureT);
    delayMicroseconds(20000); 

    // PM2.5
    digitalWrite(DUST_LED_PIN, LOW);
    delayMicroseconds(samplingTime);

    dustVal = ((analogRead(DUST_PIN) * (5.0 / 1024) * 0.17 - 0.1) / 1024 - 0.0356) * 120000 * 0.035; // TODO check formula
    dustVal = dustVal > 0 ? dustVal : 0;

    delayMicroseconds(deltaTime);
    digitalWrite(DUST_LED_PIN, HIGH);

    delayMicroseconds(20000); 

    // light TODO add formula and cancel time data
    light_pulse_time = pulseIn(LIGHT_PULSE_PIN, LOW);
    delayMicroseconds(20000); 
    
    SweepRadar();

    if (Wire.requestFrom((uint8_t)KEY_RADAR_ADDR, (uint8_t)1) != 1) { // request 1 byte from key radar serial
        key_radar_code = 0; // for error
    } else {
        key_radar_code = (Wire.read() & 0x08) | (key_radar_code & 0xf7); //set radar bit TODO check it
    }
    // TODO now code order use key_radar_code & 0x08 == 1 for is animal
}

// use servo to Sweep Radar, check if there is animal
// TODO inline or marco?
void SweepRadar() {
    radar_sweeper.write(analogRead(CHAOS_PIN) % 181); // turn it to 0-180 TODO change formula?
    delayMicroseconds(100); // TODO check time
    radar_sweeper.write(analogRead(CHAOS_PIN) % 181); // turn it to 0-180 TODO change formula?
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
    if (!irrecv.decode(&ir_code)) { // get nothing from IR
        return;
    }
    if (ir_code.value == ROLL_IR) {
        state ^= 0x01; // reverse P0
        irrecv.resume();
        return;
    }    
    if (state & 0x01 != 0) { // only in stop roll mode need to deal other key
        switch (ir_code.value)
        {
        case UP_IR:
            if (state & 0x70 == 0) { // the first line show temperature(code 0) now, set for circle to animal detect(code 5)
                state += 0x60;
            }
            if (state & 0x0e == 0) { // the second line show temperature(code 0) now, set for circle to animal detect(code 5)
                state += 0x0c;
            }
            state -= 0x12; // up line, both 3 bits show code -1
            break;
        case DOWN_IR:
            state += 0x12;
            if (state & 0x70 == 0x60) { // the first line show animal detect(code 5) before, set now for circle to temperature(code 0)
                state &= 0x0f;
            }
            if (state & 0x0e == 0x0c) { // the second line show animal detect(code 5) before, set now for circle to animal detect
                state &= 0xf1;
            }
            // up line, both 3 bits show code +1
            break;
        default: // self define order TODO hide other useless key
            state = state & 0x8f + (IRNum2Num(ir_code) << 4); // set the first line
            irrecv.resume();
            delay(10); //TODO whether delay
            if (!irrecv.decode(&ir_code)) {
                break;
            }
            state = state & 0x8f + (IRNum2Num(ir_code) << 1); // set the second line
            break;
        }
        irrecv.resume();
        return;
    }
    return;
}

uint8_t IRNum2Num(const decode_results& ir_num) {
    switch (ir_num.value) {
    case K0_IR:
        return 0;
    case K1_IR:
        return 1;
    case K2_IR:
        return 2;
    case K3_IR:
        return 3;
    case K4_IR:
        return 4;
    case K5_IR:
        return 5;
    default:
        break;
    }
    return 0xff; // for error TODO cancel it
}

void BluetoothSend() {
    Serial.print(t_h_sensor.temperature_int);Serial.print(F("."));Serial.print(t_h_sensor.temperature_dec);
    Serial.print(F(","));
    Serial.print(t_h_sensor.humidity_int);Serial.print(F("."));Serial.print(t_h_sensor.humidity_dec);
    Serial.print(F(","));
    Serial.print(presureP);
    Serial.print(F(","));
    Serial.print(dustVal);
    Serial.print(F(","));
    Serial.print(light_pulse_time);
    Serial.print(F(","));
    Serial.print(key_radar_code & 0x08);
    Serial.println(F(","));
}

void FileWrite() {
    record_file = SD.open(F("record.csv")); //TODO multi file name
    if (!record_file) {
        Serial.print(F("no space"));
        return;
    } 
    record_file.print(millis());
    record_file.print(F(","));
    record_file.print(t_h_sensor.temperature_int);record_file.print(F("."));record_file.print(t_h_sensor.temperature_dec);
    record_file.print(F(","));
    record_file.print(t_h_sensor.humidity_int);record_file.print(F("."));record_file.print(t_h_sensor.humidity_dec);
    record_file.print(F(","));
    record_file.print(presureP);
    record_file.print(F(","));
    record_file.print(dustVal);
    record_file.print(F(","));
    record_file.print(light_pulse_time);
    record_file.print(F(","));
    record_file.print(key_radar_code & 0x08);
    record_file.println(F(","));
    record_file.close();
    return;
}