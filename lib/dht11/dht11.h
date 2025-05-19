#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

int dht11_read(uint8_t dataPin, uint8_t* humidityPercent, uint8_t* temperatureCelsius);

#endif // DHT11_H
