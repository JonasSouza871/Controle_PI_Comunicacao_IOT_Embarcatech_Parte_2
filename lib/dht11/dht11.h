#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

// Atualiza a assinatura da função para usar float em vez de uint8_t
int dht11_read(uint8_t dataPin, float* humidityPercent, float* temperatureCelsius);

#endif // DHT11_H
