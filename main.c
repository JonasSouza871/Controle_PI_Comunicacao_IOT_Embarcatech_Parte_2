#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lib/dht11/dht11.h" //Biblioteca para o sensor de temperatura e umidade DHT11
#include "lib/Display_Bibliotecas/ssd1306.h" //Biblioteca para o display OLED SSD1306
#include "lib/Matriz_Bibliotecas/matriz_led.h" //Biblioteca para a matriz de LEDs
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"

//=== CONFIGURAÇÕES DO SISTEMA ===
//Configurações de Wi-Fi
#define WIFI_NOME_REDE "Jonas Souza"
#define WIFI_SENHA     "12345678"

//Pinos de hardware
#define PINO_DHT11     16 //Pino do sensor DHT11 (temperatura e umidade)
#define PINO_JOYSTICK_Y 26 //Pino do eixo Y do joystick (ADC0)
#define PINO_BOTAO_A    5 //Pino do botão A
#define PINO_LED_AZUL  12 //Pino do LED azul (PWM)
#define PINO_BUZZER    10 //Pino do buzzer (PWM)
#define PINO_I2C_SDA   14 //Pino SDA para I2C (OLED)
#define PINO_I2C_SCL   15 //Pino SCL para I2C (OLED)

//Configurações do display OLED
#define PORTA_I2C_OLED i2c1
#define ENDERECO_OLED  0x3C

//Parâmetros de controle
#define RPM_MINIMO     300.0f //RPM mínimo do motor simulado
#define RPM_MAXIMO     2000.0f //RPM máximo do motor simulado
#define TAMANHO_HISTORICO 60 //Tamanho do buffer de histórico de temperaturas

//=== ESTRUTURAS E VARIÁVEIS GLOBAIS ===
typedef struct {
    ssd1306_t display; //Estrutura do display OLED
    float temperaturas[TAMANHO_HISTORICO]; //Histórico de temperaturas
    int indice_temperatura; //Índice atual no buffer circular
    int contador_temperaturas; //Contador de temperaturas armazenadas
    float temperatura_ambiente; //Temperatura atual lida do DHT11
    float umidade_ambiente; //Umidade atual lida do DHT11
    int setpoint_temperatura; //Temperatura desejada (setpoint)
    uint16_t ciclo_pwm; //Ciclo de trabalho do PWM (0 a 65535)
    float rpm_atual; //RPM simulado do motor
    bool modo_selecao; //Indica se está ajustando o setpoint
    bool tela_principal; //Indica se exibe a tela principal no OLED
    bool sistema_ligado; //Indica se o sistema de controle está ativo
    uint canal_pwm_led; //Canal PWM do LED azul
    uint fatia_pwm_led; //Fatia PWM do LED azul
} EstadoSistema;

//Variável global para o estado do sistema
static EstadoSistema estado = {
    .indice_temperatura = 0,
    .contador_temperaturas = 0,
    .temperatura_ambiente = 0.0f,
    .umidade_ambiente = 0.0f,
    .setpoint_temperatura = 20,
    .ciclo_pwm = 0,
    .rpm_atual = RPM_MINIMO,
    .modo_selecao = true,
    .tela_principal = true,
    .sistema_ligado = false
};

//=== FUNÇÕES AUXILIARES ===
void inicializar_hardware(void) {
    //Inicializa comunicação serial
    stdio_init_all();
    
    //Inicializa matriz de LEDs
    inicializar_matriz_led();

    //Configura I2C para o display OLED
    i2c_init(PORTA_I2C_OLED, 400000);
    gpio_set_function(PINO_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PINO_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_I2C_SDA);
    gpio_pull_up(PINO_I2C_SCL);
    
    //Inicializa o display OLED
    ssd1306_init(&estado.display, 128, 64, false, ENDERECO_OLED, PORTA_I2C_OLED);
    ssd1306_config(&estado.display);

    //Configura PWM para o LED azul
    gpio_set_function(PINO_LED_AZUL, GPIO_FUNC_PWM);
    estado.fatia_pwm_led = pwm_gpio_to_slice_num(PINO_LED_AZUL);
    estado.canal_pwm_led = pwm_gpio_to_channel(PINO_LED_AZUL);
    pwm_set_wrap(estado.fatia_pwm_led, 65535);
    pwm_set_chan_level(estado.fatia_pwm_led, estado.canal_pwm_led, 0);
    pwm_set_enabled(estado.fatia_pwm_led, true);
}

float calcular_media_temperaturas(void) {
    //Calcula a média das temperaturas armazenadas no histórico
    float soma = 0.0f;
    for (int i = 0; i < estado.contador_temperaturas; i++) {
        soma += estado.temperaturas[i];
    }
    return estado.contador_temperaturas ? (soma / estado.contador_temperaturas) : 0.0f;
}

void atualizar_tela_oled_selecao(void) {
    //Exibe a tela de ajuste de setpoint no OLED
    char texto[32];
    ssd1306_fill(&estado.display, false);
    ssd1306_draw_string(&estado.display, "Ajuste Setpoint:", 0, 0, false);
    snprintf(texto, sizeof(texto), "   %2d °C", estado.setpoint_temperatura);
    ssd1306_draw_string(&estado.display, texto, 0, 16, false);
    ssd1306_draw_string(&estado.display, "[A] Confirma", 0, 32, false);
    ssd1306_send_data(&estado.display);
}

void atualizar_tela_oled_principal(void) {
    //Exibe informações principais (temperatura, setpoint, erro, PWM)
    char texto[32];
    ssd1306_fill(&estado.display, false);
    snprintf(texto, sizeof(texto), "Temp: %4.1f °C", estado.temperatura_ambiente);
    ssd1306_draw_string(&estado.display, texto, 0, 0, false);
    snprintf(texto, sizeof(texto), "Set:  %3d °C", estado.setpoint_temperatura);
    ssd1306_draw_string(&estado.display, texto, 0, 16, false);
    snprintf(texto, sizeof(texto), "Erro: %4.1f °C", (float)estado.setpoint_temperatura - estado.temperatura_ambiente);
    ssd1306_draw_string(&estado.display, texto, 0, 32, false);
    snprintf(texto, sizeof(texto), "PWM:  %5u", estado.ciclo_pwm);
    ssd1306_draw_string(&estado.display, texto, 0, 48, false);
    ssd1306_send_data(&estado.display);

    //Atualiza a matriz de LEDs com base no erro
    float erro = fabsf((float)estado.setpoint_temperatura - estado.temperatura_ambiente);
    int parte_inteira = (int)floorf(erro);
    float parte_decimal = erro - parte_inteira;
    int digito = (parte_decimal >= 0.6f) ? parte_inteira + 1 : parte_inteira;
    uint32_t cor;

    //Define a cor da matriz de LEDs com base no erro
    switch (digito) {
        case 0: cor = COR_BRANCO; break;
        case 1: cor = COR_PRATA; break;
        case 2: cor = COR_CINZA; break;
        case 3: cor = COR_VIOLETA; break;
        case 4: cor = COR_AZUL; break;
        case 5: cor = COR_MARROM; break;
        case 6: cor = COR_VERDE; break;
        case 7: cor = COR_OURO; break;
        case 8: cor = COR_LARANJA; break;
        case 9: cor = COR_AMARELO; break;
        default: cor = COR_OFF; break;
    }
    if (erro > 9.6f) {
        matriz_draw_pattern(PAD_X, COR_VERMELHO);
    } else {
        matriz_draw_number(digito, cor);
    }
}

void atualizar_tela_oled_rpm(void) {
    //Exibe informações de RPM e status no OLED
    char texto[32];
    ssd1306_fill(&estado.display, false);
    snprintf(texto, sizeof(texto), "RPM: %4.0f", estado.rpm_atual);
    ssd1306_draw_string(&estado.display, texto, 0, 0, false);

    //Desenha uma barra proporcional ao RPM
    float faixa = RPM_MAXIMO - RPM_MINIMO;
    float posicao = (estado.rpm_atual - RPM_MINIMO) / faixa;
    int comprimento = (int)(posicao * estado.display.width);
    for (int i = 0; i < comprimento; i++) {
        ssd1306_line(&estado.display, i, 20, i, 25, true);
    }

    snprintf(texto, sizeof(texto), "Min:%4.0f Max:%4.0f", RPM_MINIMO, RPM_MAXIMO);
    ssd1306_draw_string(&estado.display, texto, 0, 32, false);
    const char *mensagem = (estado.temperatura_ambiente > estado.setpoint_temperatura) ? "ESFRIAR!!" : "ESQUENTAR!!";
    ssd1306_draw_string(&estado.display, mensagem, 0, 50, false);
    ssd1306_send_data(&estado.display);
}

//=== taskS DO FreeRTOS ===
void task_leitura_sensor(void *parametros) {
    //Configura o pino do sensor DHT11
    gpio_init(PINO_DHT11);
    
    while (true) {
        if (estado.sistema_ligado) {
            float umidade, temperatura;
            //Lê temperatura e umidade do sensor DHT11
            if (dht11_read(PINO_DHT11, &umidade, &temperatura) == 0) {
                estado.temperatura_ambiente = temperatura;
                estado.umidade_ambiente = umidade;
                //Armazena a temperatura no buffer circular
                estado.temperaturas[estado.indice_temperatura] = temperatura;
                estado.indice_temperatura = (estado.indice_temperatura + 1) % TAMANHO_HISTORICO;
                if (estado.contador_temperaturas < TAMANHO_HISTORICO) {
                    estado.contador_temperaturas++;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); //Aguarda 1 segundo
    }
}

void task_entrada_usuario(void *parametros) {
    //Inicializa ADC para o joystick e configura o botão
    adc_init();
    adc_gpio_init(PINO_JOYSTICK_Y);
    adc_select_input(0);
    gpio_init(PINO_BOTAO_A);
    gpio_set_dir(PINO_BOTAO_A, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_A);

    bool botao_anterior = false;
    int direcao_anterior = 0;

    while (true) {
        //Lê o valor do joystick (eixo Y) e o estado do botão
        uint16_t valor_adc = adc_read();
        bool botao_atual = (gpio_get(PINO_BOTAO_A) == 0);
        int direcao = (valor_adc > 3000 ? 1 : (valor_adc < 1000 ? -1 : 0));

        //Ajusta o setpoint apenas no modo de seleção e com sistema desligado
        if (estado.modo_selecao && !estado.sistema_ligado) {
            if (direcao == 1 && direcao_anterior == 0 && estado.setpoint_temperatura < 30) {
                estado.setpoint_temperatura++;
            }
            if (direcao == -1 && direcao_anterior == 0 && estado.setpoint_temperatura > 10) {
                estado.setpoint_temperatura--;
            }
            if (botao_atual && !botao_anterior) {
                estado.modo_selecao = false;
                estado.sistema_ligado = true;
            }
        } else if (botao_atual && !botao_anterior) {
            estado.modo_selecao = true;
            estado.sistema_ligado = false;
        }

        botao_anterior = botao_atual;
        direcao_anterior = direcao;
        vTaskDelay(pdMS_TO_TICKS(100)); //Aguarda 100ms para debounce
    }
}

void task_controle_pi(void *parametros) {
    //Parâmetros do controlador PI
    const float kp = 120.0f; //Ganho proporcional
    const float ki = 120.0f / 15.0f; //Ganho integral
    const float intervalo = 1.0f; //Intervalo de amostragem (1s)
    float integral = 0.0f;

    pwm_set_chan_level(estado.fatia_pwm_led, estado.canal_pwm_led, 0);

    while (true) {
        if (estado.sistema_ligado) {
            //Calcula o erro entre a temperatura desejada e a atual
            float erro = estado.temperatura_ambiente - (float)estado.setpoint_temperatura;
            float termo_proporcional = kp * erro;
            integral += ki * erro * intervalo;
            integral = fmaxf(fminf(integral, 4096.0f), -4096.0f); //Limita o termo integral

            //Calcula o sinal de controle e converte para ciclo de trabalho PWM
            float sinal_controle = termo_proporcional + integral;
            int32_t ciclo = (int32_t)((sinal_controle + 4096.0f) * (65535.0f / 8192.0f));
            ciclo = ciclo < 0 ? 0 : (ciclo > 65535 ? 65535 : ciclo);
            estado.ciclo_pwm = ciclo;

            //Atualiza o RPM simulado com base no ciclo PWM
            estado.rpm_atual = RPM_MINIMO + (RPM_MAXIMO - RPM_MINIMO) * (estado.ciclo_pwm / 65535.0f);
            pwm_set_chan_level(estado.fatia_pwm_led, estado.canal_pwm_led, estado.ciclo_pwm);
        } else {
            //Reseta o controlador e desliga o PWM quando o sistema está desligado
            integral = 0.0f;
            estado.ciclo_pwm = 0;
            estado.rpm_atual = RPM_MINIMO;
            pwm_set_chan_level(estado.fatia_pwm_led, estado.canal_pwm_led, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); //Aguarda 1 segundo
    }
}

void task_buzzer_alerta(void *parametros) {
    //Configura o buzzer como saída PWM
    gpio_set_function(PINO_BUZZER, GPIO_FUNC_PWM);
    uint fatia_pwm = pwm_gpio_to_slice_num(PINO_BUZZER);
    uint canal_pwm = pwm_gpio_to_channel(PINO_BUZZER);
    pwm_set_enabled(fatia_pwm, true);

    while (true) {
        if (estado.sistema_ligado && !estado.modo_selecao) {
            //Calcula o erro absoluto da temperatura
            float erro = fabsf(estado.temperatura_ambiente - (float)estado.setpoint_temperatura);
            if (erro > 9.6f) {
                //Alarme rápido para erro crítico
                pwm_set_clkdiv(fatia_pwm, 125.0f / 1000.0f);
                pwm_set_wrap(fatia_pwm, 1000);
                pwm_set_chan_level(fatia_pwm, canal_pwm, 500);
                pwm_set_enabled(fatia_pwm, true);
                vTaskDelay(pdMS_TO_TICKS(100));
                pwm_set_enabled(fatia_pwm, false);
                vTaskDelay(pdMS_TO_TICKS(100));
            } else if (erro >= 3.6f) {
                //Alarme moderado para erro médio
                pwm_set_clkdiv(fatia_pwm, 125.0f / 500.0f);
                pwm_set_wrap(fatia_pwm, 1000);
                pwm_set_chan_level(fatia_pwm, canal_pwm, 500);
                pwm_set_enabled(fatia_pwm, true);
                vTaskDelay(pdMS_TO_TICKS(200));
                pwm_set_enabled(fatia_pwm, false);
                vTaskDelay(pdMS_TO_TICKS(600));
            } else {
                //Alarme lento para erro baixo
                pwm_set_clkdiv(fatia_pwm, 125.0f / 200.0f);
                pwm_set_wrap(fatia_pwm, 1000);
                pwm_set_chan_level(fatia_pwm, canal_pwm, 500);
                pwm_set_enabled(fatia_pwm, true);
                vTaskDelay(pdMS_TO_TICKS(300));
                pwm_set_enabled(fatia_pwm, false);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else {
            pwm_set_enabled(fatia_pwm, false);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void task_atualizar_display(void *parametros) {
    uint32_t ultima_troca = to_ms_since_boot(get_absolute_time());

    while (true) {
        //Alterna entre telas a cada 5 segundos quando o sistema está ligado
        uint32_t agora = to_ms_since_boot(get_absolute_time());
        if (estado.sistema_ligado && !estado.modo_selecao && agora - ultima_troca > 5000) {
            estado.tela_principal = !estado.tela_principal;
            ultima_troca = agora;
        }

        //Exibe a tela apropriada com base no estado do sistema
        if (estado.modo_selecao || !estado.sistema_ligado) {
            atualizar_tela_oled_selecao();
        } else if (estado.tela_principal) {
            atualizar_tela_oled_principal();
        } else {
            atualizar_tela_oled_rpm();
        }

        vTaskDelay(pdMS_TO_TICKS(100)); //Aguarda 100ms
    }
}

static err_t callback_envio_web(void *arg, struct tcp_pcb *tpcb, uint16_t len) {
    //Fecha a conexão TCP após o envio dos dados
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t callback_recepcao_web(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    //Copia a requisição recebida
    char *requisicao = malloc(p->len + 1);
    memcpy(requisicao, p->payload, p->len);
    requisicao[p->len] = '\0';
    pbuf_free(p);

    //Processa os endpoints da requisição
    if (strncmp(requisicao, "GET /increase", 13) == 0 && !estado.sistema_ligado && estado.setpoint_temperatura < 30) {
        estado.setpoint_temperatura++;
    } else if (strncmp(requisicao, "GET /decrease", 13) == 0 && !estado.sistema_ligado && estado.setpoint_temperatura > 10) {
        estado.setpoint_temperatura--;
    } else if (strncmp(requisicao, "GET /ok", 7) == 0 && !estado.sistema_ligado) {
        estado.modo_selecao = false;
        estado.sistema_ligado = true;
    } else if (strncmp(requisicao, "GET /stop", 9) == 0 && estado.sistema_ligado) {
        estado.modo_selecao = true;
        estado.sistema_ligado = false;
    }
    free(requisicao);

    //Calcula valores para exibição
    float percentual_pwm = (estado.ciclo_pwm / 65535.0f) * 100.0f;
    float erro_temperatura = (float)estado.setpoint_temperatura - estado.temperatura_ambiente;
    float rpm_exibicao = estado.rpm_atual == RPM_MINIMO ? 0.0f : estado.rpm_atual;
    float angulo_servo = (estado.ciclo_pwm / 65535.0f) * 180.0f;
    float media_temperaturas = calcular_media_temperaturas();

    //Monta a página HTML com atualização automática a cada 2 segundos
    char corpo[2048];
    int tamanho_corpo = snprintf(corpo, sizeof(corpo),
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\">\n"
        "  <meta http-equiv=\"refresh\" content=\"2\">\n"
        "  <title>ThermoGuardian</title>\n"
        "  <style>\n"
        "    body { background-color: #b5e5fb; font-family: Arial, sans-serif; text-align: center; margin-top: 20px; }\n"
        "    h1 { font-size: 36px; margin-bottom: 20px; }\n"
        "    button { background-color: LightGray; font-size: 24px; margin: 5px; padding: 10px 20px; border-radius: 8px; }\n"
        "    .info { font-size: 20px; margin-top: 10px; color: #333; }\n"
        "    .info-container { display: inline-block; text-align: left; }\n"
        "    .status { font-weight: bold; margin: 15px; font-size: 24px; }\n"
        "    .active { color: green; }\n"
        "    .inactive { color: red; }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <h1>ThermoGuardian</h1>\n"
        "  <div class=\"status %s\">Sistema: %s</div>\n",
        estado.sistema_ligado ? "active" : "inactive",
        estado.sistema_ligado ? "ATIVO" : "INATIVO"
    );

    //Adiciona botões de controle se o sistema está desligado
    if (!estado.sistema_ligado) {
        tamanho_corpo += snprintf(corpo + tamanho_corpo, sizeof(corpo) - tamanho_corpo,
            "  <form action=\"/increase\" method=\"get\"><button type=\"submit\">+1 °C</button></form>\n"
            "  <form action=\"/decrease\" method=\"get\"><button type=\"submit\">–1 °C</button></form>\n"
            "  <form action=\"/ok\" method=\"get\"><button type=\"submit\" style=\"background-color: #90EE90;\">OK</button></form>\n"
        );
    } else {
        tamanho_corpo += snprintf(corpo + tamanho_corpo, sizeof(corpo) - tamanho_corpo,
            "  <form action=\"/stop\" method=\"get\"><button type=\"submit\" style=\"background-color: #FFCCCB;\">STOP</button></form>\n"
        );
    }

    //Adiciona informações do sistema
    tamanho_corpo += snprintf(corpo + tamanho_corpo, sizeof(corpo) - tamanho_corpo,
        "  <div class=\"info-container\">\n"
        "    <p class=\"info\">Setpoint: %d °C</p>\n"
        "    <p class=\"info\">Temperatura Medida: %.1f °C</p>\n"
        "    <p class=\"info\">Umidade Medida: %.1f %%</p>\n"
        "    <p class=\"info\">Erro Atual: %.1f °C</p>\n"
        "    <p class=\"info\">PWM LED: %u / 65535 (%.1f %%)</p>\n"
        "    <p class=\"info\">RPM Simulado (300–2000): %.0f RPM</p>\n"
        "    <p class=\"info\">Servo Motor Simulado: %.1f°</p>\n"
        "    <p class=\"info\">Temp Média Últimos %d: %.1f °C</p>\n"
        "  </div>\n"
        "</body>\n"
        "</html>\n",
        estado.setpoint_temperatura,
        estado.temperatura_ambiente,
        estado.umidade_ambiente,
        erro_temperatura,
        estado.ciclo_pwm,
        percentual_pwm,
        rpm_exibicao,
        angulo_servo,
        estado.contador_temperaturas,
        media_temperaturas
    );

    //Envia a resposta HTTP
    char cabecalho[128];
    int tamanho_cabecalho = snprintf(cabecalho, sizeof(cabecalho),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        tamanho_corpo
    );

    tcp_write(tpcb, cabecalho, tamanho_cabecalho, TCP_WRITE_FLAG_COPY);
    tcp_write(tpcb, corpo, tamanho_corpo, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_sent(tpcb, callback_envio_web);
    return ERR_OK;
}

static err_t callback_aceitar_conexao(void *arg, struct tcp_pcb *nova_conexao, err_t err) {
    //Registra a função de recepção para novas conexões
    tcp_recv(nova_conexao, callback_recepcao_web);
    return ERR_OK;
}

void task_servidor_web(void *parametros) {
    //Inicializa o módulo Wi-Fi
    if (cyw43_arch_init()) {
        printf("Falha ao iniciar Wi-Fi\n");
        vTaskDelete(NULL);
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    cyw43_arch_enable_sta_mode();

    //Conecta à rede Wi-Fi
    printf("Conectando a %s...\n", WIFI_NOME_REDE);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_NOME_REDE, WIFI_SENHA, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha na conexão\n");
        vTaskDelete(NULL);
    }
    printf("Conectado!\n");
    if (netif_default) {
        printf("IP: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    //Configura o servidor HTTP na porta 80
    struct tcp_pcb *servidor = tcp_new();
    if (!servidor || tcp_bind(servidor, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao vincular porta 80\n");
        vTaskDelete(NULL);
    }
    servidor = tcp_listen(servidor);
    tcp_accept(servidor, callback_aceitar_conexao);
    printf("Servidor HTTP iniciado na porta 80\n");

    while (true) {
        cyw43_arch_poll(); //Processa eventos de rede
        vTaskDelay(pdMS_TO_TICKS(100)); //Aguarda 100ms
    }
}

//=== FUNÇÃO PRINCIPAL ===
int main(void) {
    //Inicializa o hardware (serial, I2C, PWM, etc.)
    inicializar_hardware();

    //Cria as tasks do FreeRTOS
    xTaskCreate(task_leitura_sensor, "LeituraSensor", 256, NULL, 3, NULL);
    xTaskCreate(task_entrada_usuario, "EntradaUsuario", 512, NULL, 2, NULL);
    xTaskCreate(task_controle_pi, "ControlePI", 512, NULL, 2, NULL);
    xTaskCreate(task_atualizar_display, "AtualizarDisplay", 512, NULL, 1, NULL);
    xTaskCreate(task_buzzer_alerta, "BuzzerAlerta", 256, NULL, 1, NULL);
    xTaskCreate(task_servidor_web, "ServidorWeb", 1280, NULL, 1, NULL);

    //Inicia o escalonador do FreeRTOS
    vTaskStartScheduler();

    //Loop infinito (não deve chegar aqui)
    while (true) {
        tight_loop_contents();
    }
    return 0;
}