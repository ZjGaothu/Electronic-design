/**************SoftwareSerial patch, NOT WORK TODO******************* */
// #include<SoftwareSerial.h>
// SoftwareSerial bluetooth(2, 4);

// unsigned long previousMillis = 0;
// unsigned long interval = 500;
// int ledState = LOW;

// void setup() {
//     pinMode(13, OUTPUT);
//     bluetooth.begin(9600);
//     delay(200);
// }

// void loop() {
//     if (bluetooth.available()) {
//         bluetooth.write(bluetooth.read());
//     }

//     unsigned long currentMillis = millis();

//     if (currentMillis - previousMillis >= interval) {
//         previousMillis = currentMillis;

//         if (ledState == LOW) {
//             ledState = HIGH;
//         } else {
//             bluetooth.println("Arduino bluetooth");
//             ledState = LOW;
//         }

//         digitalWrite(13, ledState);
//     }
// }

/**************pin connectiion ***********************/
/*
3.3V, Arduino tx -- HC06 Rx better convert 5 - 3.3
remember disconnect when upload programme
Arduino     HC06
1(TX)       RX
0(RX)       TX
*/

char serialData;

void setup() {
    pinMode(13, OUTPUT);
    Serial.begin(9600);
    delay(200);
}

void loop() {
    if (Serial.available() > 0) {

        serialData = Serial.read();

        if (serialData == '1') {
            Serial.print("Got command:");
            Serial.println(serialData);
            Serial.println("LED-ON");

            digitalWrite(13, HIGH);
        } else {
            Serial.print("Got command:");
            Serial.println(serialData);
            Serial.println("LED-OFF");

            digitalWrite(13, LOW);
        }
        delay(100);
    }
}