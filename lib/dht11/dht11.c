#include "dht11.h"
#include "pico/stdlib.h"

// Constantes para configuração do DHT11
#define MAX_WAIT_TIME_US      1000  //Timeout em microsegundos para espera de nível
#define START_SIGNAL_LOW_MS   20    //Duração do sinal baixo de início (ms)
#define START_SIGNAL_HIGH_US  30    //Duração do sinal alto de início (µs)
#define DATA_BITS             40    //Número total de bits de dados
#define DATA_BYTES            5     //Número de bytes de dados (40 bits / 8)
#define PULSE_THRESHOLD_US    40    //Limiar para distinguir bit 0 de bit 1 (µs)

// Códigos de erro
#define ERROR_TIMEOUT         -1  //Erro de timeout ao esperar nível
#define ERROR_CHECKSUM        -2  //Erro de verificação de checksum

// Aguarda até o pino atingir o nível especificado ou retorna erro se exceder o timeout
static int waitForPinLevel(uint8_t dataPin, bool level, uint32_t timeoutUs) {
    uint32_t startTime = to_us_since_boot(get_absolute_time());
    while (gpio_get(dataPin) != level) {
        if (to_us_since_boot(get_absolute_time()) - startTime > timeoutUs) {
            return ERROR_TIMEOUT;
        }
    }
    return 0;
}

// Inicia a comunicação com o DHT11 enviando o sinal de início
static int sendStartSignal(uint8_t dataPin) {
    //Configura o pino como saída e envia sinal baixo
    gpio_set_dir(dataPin, GPIO_OUT);
    gpio_put(dataPin, 0);
    sleep_ms(START_SIGNAL_LOW_MS);

    //Envia sinal alto e configura como entrada
    gpio_put(dataPin, 1);
    sleep_us(START_SIGNAL_HIGH_US);
    gpio_set_dir(dataPin, GPIO_IN);

    return 0;
}

// Verifica a sequência de resposta do DHT11 (LOW, HIGH, LOW)
static int waitForResponse(uint8_t dataPin) {
    if (waitForPinLevel(dataPin, 0, MAX_WAIT_TIME_US) < 0) {
        return ERROR_TIMEOUT;
    }
    if (waitForPinLevel(dataPin, 1, MAX_WAIT_TIME_US) < 0) {
        return ERROR_TIMEOUT;
    }
    if (waitForPinLevel(dataPin, 0, MAX_WAIT_TIME_US) < 0) {
        return ERROR_TIMEOUT;
    }
    return 0;
}

// Lê os 40 bits de dados do DHT11 e armazena nos bytes
static int readDataBits(uint8_t dataPin, uint8_t* data) {
    for (int i = 0; i < DATA_BITS; i++) {
        //Aguarda início do bit (subida de nível)
        if (waitForPinLevel(dataPin, 1, MAX_WAIT_TIME_US) < 0) {
            return ERROR_TIMEOUT;
        }

        //Mede duração do nível alto
        uint32_t startTime = to_us_since_boot(get_absolute_time());
        if (waitForPinLevel(dataPin, 0, MAX_WAIT_TIME_US) < 0) {
            return ERROR_TIMEOUT;
        }
        uint32_t pulseLength = to_us_since_boot(get_absolute_time()) - startTime;

        //Armazena bit: 1 se duração > limiar, 0 caso contrário
        data[i / 8] <<= 1;
        if (pulseLength > PULSE_THRESHOLD_US) {
            data[i / 8] |= 1;
        }
    }
    return 0;
}

// Verifica o checksum dos dados recebidos
static int verifyChecksum(const uint8_t* data) {
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return ERROR_CHECKSUM;
    }
    return 0;
}

// Lê temperatura e umidade do sensor DHT11
int dht11_read(uint8_t dataPin, float* humidityPercent, float* temperatureCelsius) {
    uint8_t data[DATA_BYTES] = {0};

    //Inicia comunicação com o sensor
    if (sendStartSignal(dataPin) < 0) {
        return ERROR_TIMEOUT;
    }

    //Aguarda resposta do sensor
    if (waitForResponse(dataPin) < 0) {
        return ERROR_TIMEOUT;
    }

    //Lê os 40 bits de dados
    if (readDataBits(dataPin, data) < 0) {
        return ERROR_TIMEOUT;
    }

    //Verifica checksum
    if (verifyChecksum(data) < 0) {
        return ERROR_CHECKSUM;
    }

    //Atribui valores de umidade e temperatura com uma casa decimal simulada
    *humidityPercent = data[0] + (data[1] / 10.0);
    *temperatureCelsius = data[2] + (data[3] / 10.0);
    return 0;
}
