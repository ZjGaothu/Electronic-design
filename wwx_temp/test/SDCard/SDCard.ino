/************pin connection ********************/
/*
MOSI  -  11
MISO  -  12
SCK   -  13
CS    -  4
*/

#include<SPI.h>
#include<SD.h>

File RecordFile;   //large variable 1008bytes(49%)

// Serial use to check state, in real use can cancel
void setup() {
    Serial.begin(9600);
    while (!Serial) {}

    Serial.print("Init SD card...");

    if (!SD.begin(4)) {// confige address 4 as SD card and choose, CS as address
        Serial.println("Init fail");
        while (1); //check use TODO cancel it
    }
    Serial.println("Init done");

    RecordFile = SD.open("test.txt");
    if (RecordFile) {
        Serial.print("Writing..."); //TODO cancel it
        RecordFile.println("testing 1, 2, 3"); //file operation if not exist create, in Cup name/ if exist append in back
        RecordFile.close();
        Serial.println("done.");
    } else {
        Serial.println("error opening test.txt");
    }
}

void loop() {

}
