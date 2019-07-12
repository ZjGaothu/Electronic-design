// 引脚说明
/*
dht11 - 8 temperature and humidity sensor
GP2Y1050AU0F signal - A0(remember filter); led - 6
light_pulse - 7
IR - 9
servo - 5
chaos - A1
SD MOSI - 11; MISO - 12; SCK - 13; CS - 4
I2C device
A4 - SDA, A5 - SCK
*/
#include <Wire.h> //可模拟I2C通信，用于GPIO转I2C模块PCF8574使用
#include<Adafruit_BMP085.h>
#include "dht11.h"
#include "LiquidCrystal_I2C.h"
#include <IRremote.h>
#include <Servo.h>
#include <SPI.h>
#include <SD.h>

#define T_H_PIN 8 // 温湿度传感器DHT11
#define DUST_PIN A0 // 灰尘传感器GP2Y1050AU0F输出
#define DUST_LED_PIN 6 // 灰尘传感器探测管驱动
#define LIGHT_PULSE_PIN 7 // 555光强测量电路输出
#define IR_PIN 9 // 红外接收管信号线
#define SERVO_PIN 5 //舵机信号线
#define CHAOS_PIN A1 // 混沌电路输出
#define SD_CS_PIN 4  // SD卡位选端
#define REFRESH_INTR_PIN 3 // 接中断源，用于刷新输出

// I2C use hard I2C in arduino A4 - SDA, A5 - SCK
// I2C device, define address
#define LCD_ADDR 0x3f
#define KEY_RADAR_ADDR 0X20 // PCF8574, GPIO to I2C as a serial IO
#define AIR_PRESURE_ADDR 0x77 // SFE_BMP180

// 所用时间常数定义
// us, GP2Y采样时间参数
#define samplingTime 280
#define deltaTime 40
#define sleepTime 9680
//ms
#define show_refresh_time 700

//雷达扫描次数
#define radar_times 4

// 红外遥控键值
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

dht11 t_h_sensor;
Adafruit_BMP085 air_presure_sensor;
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
IRrecv irrecv(IR_PIN);
Servo radar_sweeper;
File record_file;

// 待测量和红外键值的存储
float dustVal = 0;
double presure;
unsigned int light_pulse_time = 0;
float light_log = 0;
decode_results ir_code;

uint8_t key_radar_code; 
//IO转I2C，机械键盘输入值与雷达扫描值一并串行输入
// 编码设定,P3雷达扫描结果
/*
P7 - P4 键盘
上翻 下翻 开始/停止滚动 灵敏度调节
*/

// 一字节状态字设置
/*
P7 - P0
自动滚屏显示 xxxx xxx0
手动翻页显示 xxxx xxx1
(手动翻页下) P6 - P4, P3 - P1,第1第2行显示的测量量
000 温度
001 湿度
010 气压
011 PM2.5
100 光强
101 专注度
*/

uint8_t state; 
bool is_page_change = 0;
uint8_t radar_count = 0;


void RrfreshMeasure();
void SweepRadar();
void Btn2State();
void PrintLCDByState();
void DecodeAndPrint(uint8_t print_code);
void IR2State();
uint8_t IRNum2Num(const decode_results& ir_num);
void BluetoothSend();
void FileWrite();

void setup() {
    Serial.begin(9600);
    // Serial.println(F("setup begin"));
    Wire.begin(); // 建立I2C通信，调用AVR底层的I2C支持，用A4A5口通信
    air_presure_sensor.begin();
    //    irrecv.enableIRIn(); // 与舵机控制同用一定时器，有串扰暂时放弃
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
    state = 0x02; // 初态设定，滚屏，若停止滚屏则第一行显示温度第二行湿度
    pinMode(REFRESH_INTR_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(REFRESH_INTR_PIN), RrfreshMeasure, RISING);
}

void loop() {
    if ((state & 0x80) != 0) {
        state &= 0x7f;
        is_page_change = 1;
        // bluetooth send
        BluetoothSend();
        // write in SD card
        FileWrite();
    }
    

    if (Wire.requestFrom((uint8_t)KEY_RADAR_ADDR, (uint8_t)1) != 1) { // request 1 byte from key radar serial
        key_radar_code = 0x07; // 错误指示
        Serial.println(F("no addr"));
    } else {
        key_radar_code = Wire.read(); // get both radar for animal data and key data
    }
    Btn2State();
    PrintLCDByState();
    delay(100);
}

void RrfreshMeasure() {
    state |= 0x80;
    // 温湿度
    t_h_sensor.read(T_H_PIN); //向DHT11发送检测请求，测量值在t_h_senor的四个数据属性中
    
    // 气压测量
    presure = air_presure_sensor.readPressure();

    // PM2.5
    digitalWrite(DUST_LED_PIN, LOW); // 点亮红外管，开始探测。负逻辑，详见datasheet电路
    delayMicroseconds(samplingTime);

    dustVal = float(analogRead(DUST_PIN)) * 1.72 - 34.34; // 换算为AQI
    dustVal = dustVal > 0 ? dustVal : 0;

    delayMicroseconds(deltaTime);
    digitalWrite(DUST_LED_PIN, HIGH);
    delayMicroseconds(sleepTime);

    // 光强测量
    light_pulse_time = pulseIn(LIGHT_PULSE_PIN, LOW); // 计算低电平时长，正比于光敏电阻值
    light_log = 0;  //计算光强的常用对数lgI，方便表示为科学计数法
    while(light_pulse_time / 10) {
    light_log++;
    light_pulse_time /= 10;  
    }
    light_log = -2.28 * light_log - 0.5 * light_pulse_time + 9.9914;
    // 近似计算，公式如下，存储光强对数值，用小的变量空间存储大范围测量值：
    // 近似计算常用对数时，先根据十进制位数得到对数整数位，再用泰勒做线性近似得到小数位
    // lg I = A * lg(t) + [- A * lg(t_0) + lg I_0], I_0 = 1e^4 lux t_0 = 6.51 us A = -1.28
    // lg t \approx (t - 1) * 2.3, use for decimal
    // 恢复公式，用于显示，以科学计数法表达，对数整数位为10的幂次，有效数字为可用指数函数线性近似表示
    // 10^x approx 1 + ln(10) * x approx 1 + 2.3 * x
    SweepRadar();       
}

void SweepRadar() {
    radar_count = 0;
    for (int i = 0; i < radar_times; i++) {
        radar_sweeper.write(analogRead(CHAOS_PIN) % 181); // turn it to 0-180 TODO change formula?
        
        if (Wire.requestFrom((uint8_t)KEY_RADAR_ADDR, (uint8_t)1) != 1) {// 向IO转I2C芯片PCF8574请求一字节数据
            key_radar_code = 0x07; // 错误指示
            Serial.println(F("no addr"));
        } else {
            key_radar_code = Wire.read();
        }
        if ((key_radar_code & 0x08) == 1) {
            radar_count++;
        }
        delayMicroseconds((unsigned long int)200000); // 200ms
    }
}

void Btn2State() {
    if ((key_radar_code & 0x20) != 0) {
        if ((state & 0x01) == 0) {
          is_page_change = 1;
          state = 0x03; //停止滚动，第一行显示温度，第二行显示湿度
          return;
        } else {
          state = 0x02;
          return;  
        }
    }
    if ((state & 0x01 != 0) && ((key_radar_code & 0x80) != 0)) { // 上翻，第一行第二行显示控制字符在0-5循环-1即可
        is_page_change = 1;
        if ((state & 0x70) == 0) { // 第一行显示控制字符为0，先+6，之后减1可以到5实现少资源的循环减1
            state += 0x60;
        }
        if ((state & 0x0e) == 0) { // 第二行显示控制字符为0，处理同上
            state += 0x0c;
        }
        state -= 0x12; // 在P6-P4, P3-P1各减1
        return;
    }
    if (((state & 0x01) != 0) && ((key_radar_code & 0x40) != 0)) { // down
        is_page_change = 1; 
        state += 0x12; // 在P6-P4, P3-P1各加1
        if ((state & 0x70) == 0x60) { // 第一行显示控制字符原为5现为6，-1实现循环5-0的循环
            state &= 0x0f;
        }
        if ((state & 0x0e) == 0x0c) { //  第二行显示控制字符原为5现为6，同上
            state &= 0xf1;
        }
         return;
    }
    return;
}

void PrintLCDByState() {
    if ((state & 0x01) == 1) { // stop roll
        if (!is_page_change) {
            return;
        }
        is_page_change = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        DecodeAndPrint((state & 0x70) >> 4); // show the first line
        lcd.setCursor(0, 1);
        DecodeAndPrint((state & 0x0e) >> 1); //show the second line
        return;
    } else {
        // 温湿度显示
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Temp:"));
        lcd.print(t_h_sensor.temperature_int);
        lcd.print(F("."));
        lcd.print(t_h_sensor.temperature_dec);
        lcd.print((char)223); // 摄氏度上标的圆圈
        lcd.print(F("C "));
        lcd.setCursor(0, 1);
        lcd.print(F("Hum: "));
        lcd.print(t_h_sensor.humidity_int);
        lcd.print(F("."));
        lcd.print(t_h_sensor.humidity_dec);
        delay(show_refresh_time);

        // 气压与PM2.5显示
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("AirP:"));
        lcd.print(presure);
        lcd.print(F("Pa"));
        lcd.setCursor(0, 1);
        lcd.print(F("PM2.5:"));
        lcd.print(dustVal);
        delay(show_refresh_time);

        //光强和雷达生物检测(专注度)显示
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("I:"));
        lcd.print(1 + (light_log - (int)light_log) * 2.4); //把光强常用对数值换算为显示
        lcd.print(F("e"));
        lcd.print((int)light_log);
        lcd.print(F(" lx"));
        lcd.setCursor(0, 1);
        lcd.print(F("isFocus:"));
        lcd.print(radar_count);
        lcd.print(F("/"));
        lcd.print(radar_times);
        delay(show_refresh_time);
        delay(show_refresh_time);
        return;
    }
}

void DecodeAndPrint(uint8_t code) {
    switch (code) {
        case 0x00: // 温度
            lcd.print(F("Temp:"));
            lcd.print(t_h_sensor.temperature_int);
            lcd.print(F("."));
            lcd.print(t_h_sensor.temperature_dec);
            lcd.print((char)223); // 摄氏度上标的圆圈
            lcd.print(F("C "));
            return;
        case 0x01: // 湿度
            lcd.print(F("Hum: "));
            lcd.print(t_h_sensor.humidity_int);
            lcd.print(F("."));
            lcd.print(t_h_sensor.humidity_dec);
            return;
        case 0x02: // 气压
            lcd.print(F("AirP:"));
            lcd.print(presure);
            lcd.print(F("Pa"));
            return;
        case 0x03: // PM2.5
            lcd.print(F("PM2.5:"));
            lcd.print(dustVal);
            return;
        case 0x04: // 光强
            lcd.print(F("I:"));
            lcd.print(1 + (light_log - (int)light_log) * 2.4); 
            lcd.print(F("e"));
            lcd.print((int)light_log);
            lcd.print(F("lx"));
            return;
        case 0x05: // 专注度
            lcd.print(F("isFocus:"));
            lcd.print(radar_count);
            lcd.print(F("/"));
            lcd.print(radar_times);
            return;
        default:
            return;
        }
}

void IR2State() {
    if (!irrecv.decode(&ir_code)) { // 未接收红外数值
        return;
    }
    Serial.print(ir_code.value,HEX);
    if (ir_code.value == ROLL_IR) {
        state ^= 0x01; // 接受到开始停止滚动按键，P0取反其他位不变
        irrecv.resume();
        return;
    }    
    if (state & 0x01 != 0) { // 停止滚屏显示下，才处理翻页操作
        switch (ir_code.value)
        {
        case UP_IR:
            if ((state & 0x70) == 0) { // 第一行为0，先+6再-1实现循环
                state += 0x60;
            }
            if ((state & 0x0e) == 0) { // 第二行为0，同上
                state += 0x0c;
            }
            state -= 0x12; 
            break;
        case DOWN_IR:
            state += 0x12;
            if ((state & 0x70) == 0x60) { // 第一行显示控制字符原为5现为6，需要置零实现循环
                state &= 0x0f;
            }
            if ((state & 0x0e) == 0x0c) { //  第二行显示控制字符原为5现为6，需要置零实现循环
                state &= 0xf1;
            }
            break;
        default: // 红外还支持按两次输入的数字，自定义显示的物理量
            state = (state & 0x8f) + (IRNum2Num(ir_code) << 4); // 根据读数置位第一行显示控制字符
            irrecv.resume();
            delay(1);
            if (!irrecv.decode(&ir_code)) {
                break;
            }
            state = (state & 0x8f) + (IRNum2Num(ir_code) << 1); // 根据读数置位第二行显示控制字符
            break;
        }
        irrecv.resume();
        return;
    } else {
      irrecv.resume();  
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
    return 0xff; // 未定义按键返回错误码
}

void BluetoothSend() {
    // .csv文件格式传输数据，用逗号分隔，文本换行表格换行
    Serial.print(F(",")); // 上位机接受数据保存会加上空格分隔的时间戳，加入一个逗号，可以让时间作为上位机存储的csv文件的一列
    Serial.print(t_h_sensor.temperature_int);Serial.print(F("."));Serial.print(t_h_sensor.temperature_dec);
    Serial.print(F(","));
    Serial.print(t_h_sensor.humidity_int);Serial.print(F("."));Serial.print(t_h_sensor.humidity_dec);
    Serial.print(F(","));
    Serial.print(presure);
    Serial.print(F(","));
    Serial.print(dustVal);
    Serial.print(F(","));
    Serial.print(light_log); // 数据记录，传递对数光强值，格式紧凑，也符合光强的一般记录方法
    Serial.print(F(","));
    Serial.print((key_radar_code >> 3) & 0x01);
    Serial.println(F(","));
}

void FileWrite() {
    record_file = SD.open(F("record.csv"), FILE_WRITE); 
    if (!record_file) {
        Serial.print(F("no space"));
        return;
    }
    // SD卡作为备份数据记录，也按.csv格式存储，时间列按arduino运行时间保存，故会有溢出复位
    // 用备份文件处理时需要统计软件对时间进行预处理,(在时间下降时，给后续单元格加上时间(unsigned long)上限4294967295)
    record_file.print(millis());
    record_file.print(F(","));
    record_file.print(t_h_sensor.temperature_int);record_file.print(F("."));record_file.print(t_h_sensor.temperature_dec);
    record_file.print(F(","));
    record_file.print(t_h_sensor.humidity_int);record_file.print(F("."));record_file.print(t_h_sensor.humidity_dec);
    record_file.print(F(","));
    record_file.print(presure);
    record_file.print(F(","));
    record_file.print(dustVal);
    record_file.print(F(","));
    record_file.print(light_log); // 数据记录，传递对数光强值，格式紧凑，也符合光强的一般记录方法
    record_file.print(F(","));
    record_file.print(key_radar_code & 0x08);
    record_file.println(F(","));
    record_file.close();
    return;
}
