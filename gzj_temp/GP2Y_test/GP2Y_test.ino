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
 
  calcVoltage = voMeasured*(5.0/1024);
  dustDensity = 0.17*calcVoltage-0.1;
 
  if ( dustDensity < 0)
  {
    dustDensity = 0.00;
  }
 
  Serial.println("Raw Signal Value (0-1023):");
  Serial.println(voMeasured);
 
  Serial.println("Voltage:");
  Serial.println(calcVoltage);
 
  Serial.println("Dust Density:");
  Serial.println(dustDensity);
 
  delay(20);
}
//数据端A0
