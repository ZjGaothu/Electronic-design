#define light_555_pin 2

unsigned long duration; //TODO less bits?
void setup() {
    Serial.begin(9600);
    pinMode(light_555_pin, INPUT);
}

void loop() {
    duration = pulseIn(light_555_pin, LOW);
    Serial.println(duration);
    delay(1000);
}
