#include <Servo.h>

Servo RadarSweeper;

int pos = 0;

void setup() {
    RadarSweeper.attach(9); // signal pin confige, need pin support PWM TODO
}

void loop() {
    RadarSweeper(pos + 5); // digital degree control 0-180 directly
}