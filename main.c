#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/dht11/dht11.h"

#define DHT11_PIN 16  // ajuste para o GPIO que você estiver usando

int main() {
    stdio_init_all();       // inicializa UART/USB serial
    sleep_ms(2000);         // espera Porta Serial subir

    gpio_init(DHT11_PIN);
    while (1) {
        uint8_t h = 0, t = 0;
        if (dht11_read(DHT11_PIN, &h, &t) == 0) {
            printf("Umidade: %u%%   Temperatura: %u°C\n", h, t);
        } else {
            printf("Falha ao ler DHT11\n");
        }
        sleep_ms(2000);  // aguarda 2 segundos entre leituras
    }
    return 0;
}
