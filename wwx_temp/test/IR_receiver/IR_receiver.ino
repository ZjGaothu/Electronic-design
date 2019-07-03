// receive and check remote controller code
#include <IRremote.h>

IRrecv irrecv(6);

decode_results result; // 427btye(20%) variable memory

void setup() {
    Serial.begin(9600);
    irrecv.enableIRIn();
    Serial.println("Enabled IRin");
}

void loop() {
    if (irrecv.decode(&result)) {
        Serial.println(result.value, HEX);
        irrecv.resume();    // recover from block use to be ready for next check
    }
    delay(100);
}

//result
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