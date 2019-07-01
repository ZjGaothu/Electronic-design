#define MAXLENGTH 32
char comchar; // char by char serial

int numdata = 0;
char buffer[MAXLENGTH]; // chars serial to LCD

void setup() {
   Serial.begin(9600);
   while(Serial.read() >= 0) {} // clear serial port when begin
}


/**************************char by char patch ***********************/
// void loop() {
//    if (Serial.available() > 0) {
//       comchar = Serial.read() & 0xdf; // low to high
//       Serial.print("Serial.read: ");
//       Serial.println(comchar);
//       delay(100);
//    }
//    while(Serial.read() >= 0){}
// }

/*******************buffer patch *******************************/
// alternative func Serial.readBytesUntil(stop_char, buffer, num) stop at num or the stop_char char
// Serial.readString(), Serial.readStringUntil(stop_char); ssssslow maybe 1s
void loop() {
   if(Serial.available() > 0){
      delay(100); //TODO may change, wait a string send finish
      numdata = Serial.readBytes(buffer, MAXLENGTH);
      Serial.print("Serial readBytes: ");
      Serial.println(buffer);
   }

   // clear buffer
   while(Serial.read() >= 0){}
   for(int i = 0; i < MAXLENGTH; i++) {
      buffer[i] = '\0';
   }
}

// other useful Serial function
// serial char match func Serial.find(target_char), Serial.findUntil(target_char,stop_char)