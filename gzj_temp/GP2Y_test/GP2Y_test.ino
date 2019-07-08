int measurePin = A0;            
int ledPower = 12;              
 
const unsigned int samplingTime PROGMEM = 280;
const unsigned int deltaTime PROGMEM = 40;
const unsigned int sleepTime PROGMEM = 9680;
 
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;
 
void setup(){
  Serial.begin(9600);
  pinMode(ledPower,OUTPUT);
}
 
void loop(){
  digitalWrite(ledPower,LOW);
  delayMicroseconds(samplingTime);
 
  voMeasured = analogRead(measurePin);
 
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH);
  delayMicroseconds(sleepTime);
 
  Serial.println("Raw Signal Value (0-1023):");
  Serial.println(voMeasured);
  delay(3000);
}
//数据端A0
