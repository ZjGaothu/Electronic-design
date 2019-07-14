#ifndef dht11_h
#define dht11_h

#include <Arduino.h>


#define DHTLIB_OK				0
#define DHTLIB_ERROR_CHECKSUM	-1
#define DHTLIB_ERROR_TIMEOUT	-2

class dht11
{
public:
    int read(int pin);
	uint8_t humidity_int;
  uint8_t humidity_dec;
  uint8_t temperature_int;
  uint8_t temperature_dec;
};
#endif
//
// END OF FILE
//
