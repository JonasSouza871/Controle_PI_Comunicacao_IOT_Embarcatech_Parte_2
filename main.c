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
#include "lib/dht11/dht11.h"                       // Biblioteca do DHT11
#include "lib/Display_Bibliotecas/ssd1306.h"
#include "lib/Matriz_Bibliotecas/matriz_led.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"

// ----------------- CONFIGURAÇÃO WI-FI -----------------
#define WIFI_SSID     "Jonas Souza"
#define WIFI_PASSWORD "12345678"

// ----------------- PINOS -----------------
#define DHT11_PIN     16    // Sensor DHT11 (GPIO 16)
#define PIN_VRY       26    // Joystick vertical (ADC0)
#define PIN_BTN_A     5     // Botão A
#define PIN_RGB_B     12    // LED RGB (azul)
#define PIN_BUZZER    10    // Buzzer
#define I2C_SDA       14    // I2C SDA (OLED)
#define I2C_SCL       15    // I2C SCL (OLED)

// ----------------- CONFIGURAÇÃO OLED -----------------
#define OLED_I2C_PORT i2c1
#define OLED_ADDR     0x3C

// ----------------- VALORES DE RPM -----------------
#define RPM_MIN 300.0f
#define RPM_MAX 2000.0f

// ----------------- HISTÓRICO DE TEMPERATURAS -----------------
#define TEMP_HISTORY_SIZE 60
static float temp_history[TEMP_HISTORY_SIZE];
static int temp_index = 0;
static int temp_count = 0;

// ----------------- VARIÁVEIS GLOBAIS -----------------
static ssd1306_t oled;
static float temperatura_atual = 0.0f;    // Temperatura lida
static float umidade_atual    = 0.0f;    // Umidade lida
static int setpoint           = 20;      // °C
static volatile uint16_t duty_cycle_pwm = 0;
static float rpm_simulado     = RPM_MIN;
static bool selecionando      = true;
static bool exibir_tela_principal = true;
static uint slice_b, chan_b;
static bool sistema_ativo     = false;   // Se o controle está ativo

// ----------------- PRÓTOTIPOS -----------------
void Task_Sensor(void *pv);
void Task_Input(void *pv);
void Task_Control(void *pv);
void Task_Buzzer(void *pv);
void Task_Display(void *pv);
void Task_Webserver(void *pv);
static err_t webserver_sent(void *arg, struct tcp_pcb *tpcb, uint16_t len);
static err_t webserver_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t webserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// ============================================================================
// Task: Leitura do DHT11 e histórico de temperaturas
// ============================================================================
void Task_Sensor(void *pv) {
    gpio_init(DHT11_PIN);
    while (true) {
        if (sistema_ativo) {
            float h = 0.0f, t = 0.0f;
            if (dht11_read(DHT11_PIN, &h, &t) == 0) {
                temperatura_atual = t;
                umidade_atual     = h;
                // armazena leitura no buffer circular
                temp_history[temp_index] = t;
                temp_index = (temp_index + 1) % TEMP_HISTORY_SIZE;
                if (temp_count < TEMP_HISTORY_SIZE) temp_count++;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================================================
// Task: Ajuste de setpoint via joystick e botão
// ============================================================================
void Task_Input(void *pv) {
    adc_init();
    adc_gpio_init(PIN_VRY);
    adc_select_input(0);
    gpio_init(PIN_BTN_A);
    gpio_set_dir(PIN_BTN_A, GPIO_IN);
    gpio_pull_up(PIN_BTN_A);

    bool ultimo_btn = false;
    int ultima_dir  = 0;

    while (true) {
        uint16_t raw = adc_read();
        bool btn = (gpio_get(PIN_BTN_A) == 0);
        int dir = (raw > 3000 ? 1 : (raw < 1000 ? -1 : 0));

        if (selecionando && !sistema_ativo) {
            if (dir ==  1 && ultima_dir == 0 && setpoint < 30) setpoint++;
            if (dir == -1 && ultima_dir == 0 && setpoint > 10) setpoint--;
            if (btn && !ultimo_btn) {
                selecionando  = false;
                sistema_ativo = true;
            }
        } else {
            if (btn && !ultimo_btn) {
                selecionando  = true;
                sistema_ativo = false;
            }
        }

        ultimo_btn = btn;
        ultima_dir = dir;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// Task: Controle PI e PWM (LED azul) + simula RPM
// ============================================================================
void Task_Control(void *pv) {
    const float kp = 120.0f;
    const float ki = 120.0f / 15.0f;
    const float h  = 1.0f;
    static float integral = 0.0f;

    pwm_set_chan_level(slice_b, chan_b, 0);

    while (true) {
        if (sistema_ativo) {
            float erro = temperatura_atual - (float)setpoint;
            float P    = kp * erro;
            integral  += ki * erro * h;
            integral   = fmaxf(fminf(integral, 4096.0f), -4096.0f);

            float U = P + integral;
            int32_t duty = (int32_t)((U + 4096.0f) * (65535.0f / 8192.0f));
            duty = duty < 0 ? 0 : (duty > 65535 ? 65535 : duty);
            duty_cycle_pwm = duty;

            rpm_simulado = RPM_MIN + (RPM_MAX - RPM_MIN) * (duty_cycle_pwm / 65535.0f);
            pwm_set_chan_level(slice_b, chan_b, duty_cycle_pwm);
        } else {
            integral = 0.0f;
            duty_cycle_pwm = 0;
            pwm_set_chan_level(slice_b, chan_b, 0);
            rpm_simulado = RPM_MIN;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================================================
// Task: Buzzer de alerta conforme erro de temperatura
// ============================================================================
void Task_Buzzer(void *pv) {
    gpio_set_function(PIN_BUZZER, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_BUZZER);
    uint chan  = pwm_gpio_to_channel(PIN_BUZZER);
    pwm_set_enabled(slice, true);

    while (true) {
        if (sistema_ativo && !selecionando) {
            float erro = fabsf(temperatura_atual - (float)setpoint);
            if (erro > 9.6f) {
                pwm_set_clkdiv(slice, 125.0f / 1000.0f);
                pwm_set_wrap(slice, 1000);
                pwm_set_chan_level(slice, chan, 500);
                pwm_set_enabled(slice, true);
                vTaskDelay(pdMS_TO_TICKS(100));
                pwm_set_enabled(slice, false);
                vTaskDelay(pdMS_TO_TICKS(100));
            } else if (erro >= 3.6f) {
                pwm_set_clkdiv(slice, 125.0f / 500.0f);
                pwm_set_wrap(slice, 1000);
                pwm_set_chan_level(slice, chan, 500);
                pwm_set_enabled(slice, true);
                vTaskDelay(pdMS_TO_TICKS(200));
                pwm_set_enabled(slice, false);
                vTaskDelay(pdMS_TO_TICKS(600));
            } else {
                pwm_set_clkdiv(slice, 125.0f / 200.0f);
                pwm_set_wrap(slice, 1000);
                pwm_set_chan_level(slice, chan, 500);
                pwm_set_enabled(slice, true);
                vTaskDelay(pdMS_TO_TICKS(300));
                pwm_set_enabled(slice, false);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else {
            pwm_set_enabled(slice, false);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// ============================================================================
// Task: Atualiza OLED e matriz de LEDs
// ============================================================================
void Task_Display(void *pv) {
    char buf[32];
    uint32_t last = to_ms_since_boot(get_absolute_time());

    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (sistema_ativo && !selecionando && now - last > 5000) {
            exibir_tela_principal = !exibir_tela_principal;
            last = now;
        }

        ssd1306_fill(&oled, false);

        if (selecionando || !sistema_ativo) {
            ssd1306_draw_string(&oled, "Ajuste Setpoint:", 0, 0, false);
            snprintf(buf, sizeof(buf), "   %2d C", setpoint);
            ssd1306_draw_string(&oled, buf, 0, 16, false);
            ssd1306_draw_string(&oled, "[A] Confirma", 0, 32, false);
        } else if (exibir_tela_principal) {
            snprintf(buf, sizeof(buf), "Temp: %4.1f C", temperatura_atual);
            ssd1306_draw_string(&oled, buf, 0, 0, false);
            snprintf(buf, sizeof(buf), "Set:  %3d C", setpoint);
            ssd1306_draw_string(&oled, buf, 0, 16, false);
            snprintf(buf, sizeof(buf), "Erro: %4.1f C", (float)setpoint - temperatura_atual);
            ssd1306_draw_string(&oled, buf, 0, 32, false);
            snprintf(buf, sizeof(buf), "PWM:  %5u", duty_cycle_pwm);
            ssd1306_draw_string(&oled, buf, 0, 48, false);

            float erro = fabsf((float)setpoint - temperatura_atual);
            int parte_i  = (int)floorf(erro);
            float parte_d = erro - parte_i;
            int dig       = (parte_d >= 0.6f) ? parte_i + 1 : parte_i;
            uint32_t cor;
            switch (dig) {
                case 0: cor = COR_BRANCO;  break;
                case 1: cor = COR_PRATA;   break;
                case 2: cor = COR_CINZA;   break;
                case 3: cor = COR_VIOLETA; break;
                case 4: cor = COR_AZUL;    break;
                case 5: cor = COR_MARROM;  break;
                case 6: cor = COR_VERDE;   break;
                case 7: cor = COR_OURO;    break;
                case 8: cor = COR_LARANJA; break;
                case 9: cor = COR_AMARELO; break;
                default: cor = COR_OFF;    break;
            }
            if (erro > 9.6f) {
                matriz_draw_pattern(PAD_X, COR_VERMELHO);
            } else {
                matriz_draw_number(dig, cor);
            }
        } else {
            snprintf(buf, sizeof(buf), "RPM: %4.0f", rpm_simulado);
            ssd1306_draw_string(&oled, buf, 0, 0, false);
            float range = RPM_MAX - RPM_MIN;
            float pos   = (rpm_simulado - RPM_MIN) / range;
            int len     = (int)(pos * oled.width);
            for (int i = 0; i < len; i++) {
                ssd1306_line(&oled, i, 20, i, 25, true);
            }
            snprintf(buf, sizeof(buf), "Min:%4.0f Max:%4.0f", RPM_MIN, RPM_MAX);
            ssd1306_draw_string(&oled, buf, 0, 32, false);
            const char *act = (temperatura_atual > setpoint) ? "ESFRIAR!!" : "ESQUENTAR!!";
            ssd1306_draw_string(&oled, act, 0, 50, false);
        }

        ssd1306_send_data(&oled);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// Callbacks do webserver
// ============================================================================
static err_t webserver_sent(void *arg, struct tcp_pcb *tpcb, uint16_t len) {
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t webserver_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    // copia requisição
    char *req = malloc(p->len + 1);
    memcpy(req, p->payload, p->len);
    req[p->len] = '\0';
    pbuf_free(p);

    // trata endpoints
    if (strncmp(req, "GET /increase", 13) == 0 && !sistema_ativo && setpoint < 30) {
        setpoint++;
    } else if (strncmp(req, "GET /decrease", 13) == 0 && !sistema_ativo && setpoint > 10) {
        setpoint--;
    } else if (strncmp(req, "GET /ok", 7) == 0 && !sistema_ativo) {
        selecionando  = false;
        sistema_ativo = true;
    } else if (strncmp(req, "GET /stop", 9) == 0 && sistema_ativo) {
        selecionando  = true;
        sistema_ativo = false;
    }
    free(req);

    // calcula valores
    float pwm_led_percent = (duty_cycle_pwm / 65535.0f) * 100.0f;
    float erro_temp       = (float)setpoint - temperatura_atual;
    float rpm_display_web = rpm_simulado;
    if (fabsf(rpm_simulado - RPM_MIN) < 0.01f) {
        rpm_display_web = 0.0f;
    }

    // ângulo do servo simulado
    float servo_angle = (duty_cycle_pwm / 65535.0f) * 180.0f;

    // média das últimas leituras
    float sum = 0.0f;
    for (int i = 0; i < temp_count; i++) sum += temp_history[i];
    float temp_media = temp_count ? (sum / temp_count) : 0.0f;

    // monta HTML com refresh a cada 2s
    static char body[2048];
    int body_len = snprintf(body, sizeof(body),
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
        sistema_ativo ? "active" : "inactive",
        sistema_ativo ? "ATIVO" : "INATIVO"
    );

    // formulários de controle
    if (!sistema_ativo) {
        body_len += snprintf(body + body_len, sizeof(body) - body_len,
            "  <form action=\"/increase\" method=\"get\"><button type=\"submit\">+1 °C</button></form>\n"
            "  <form action=\"/decrease\" method=\"get\"><button type=\"submit\">–1 °C</button></form>\n"
            "  <form action=\"/ok\" method=\"get\"><button type=\"submit\" style=\"background-color: #90EE90;\">OK</button></form>\n"
        );
    } else {
        body_len += snprintf(body + body_len, sizeof(body) - body_len,
            "  <form action=\"/stop\" method=\"get\"><button type=\"submit\" style=\"background-color: #FFCCCB;\">STOP</button></form>\n"
        );
    }

    // container de informações
    body_len += snprintf(body + body_len, sizeof(body) - body_len,
        "  <div class=\"info-container\">\n"
        "    <p class=\"info\">Setpoint: %d °C</p>\n"
        "    <p class=\"info\">Temperatura Medida: %.1f °C</p>\n"
        "    <p class=\"info\">Umidade Medida: %.1f %%</p>\n"
        "    <p class=\"info\">Erro Atual: %.1f °C</p>\n"
        "    <p class=\"info\">PWM LED: %u / 65535 (%.1f %%)</p>\n"
        "    <p class=\"info\">RPM Simulado (300–2000): %.0f RPM</p>\n"
        "    <p class=\"info\">Servo Simulado: %.1f°</p>\n"
        "    <p class=\"info\">Temp Média Últimos %d: %.1f °C</p>\n"
        "  </div>\n"
        "</body>\n"
        "</html>\n",
        setpoint,
        temperatura_atual,
        umidade_atual,
        erro_temp,
        duty_cycle_pwm,
        pwm_led_percent,
        rpm_display_web,
        servo_angle,
        temp_count,
        temp_media
    );

    // cabeçalho HTTP
    char header[128];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        body_len
    );

    tcp_write(tpcb, header, header_len, TCP_WRITE_FLAG_COPY);
    tcp_write(tpcb, body,   body_len,   TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_sent(tpcb, webserver_sent);
    return ERR_OK;
}

static err_t webserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, webserver_recv);
    return ERR_OK;
}

// ============================================================================
// Task: Inicialização e loop do Wi-Fi + HTTP
// ============================================================================
void Task_Webserver(void *pv) {
    if (cyw43_arch_init()) {
        printf("Falha ao iniciar Wi-Fi\n");
        vTaskDelete(NULL);
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    cyw43_arch_enable_sta_mode();

    printf("Conectando a %s…\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
            CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha na conexão\n");
        vTaskDelete(NULL);
    }
    printf("Conectado!\n");
    if (netif_default) {
        printf("IP: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    struct tcp_pcb *srv = tcp_new();
    if (!srv || tcp_bind(srv, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro bind porta 80\n");
        vTaskDelete(NULL);
    }
    srv = tcp_listen(srv);
    tcp_accept(srv, webserver_accept);
    printf("HTTP on 80\n");

    while (true) {
        cyw43_arch_poll();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// main()
// ============================================================================
int main() {
    stdio_init_all();
    inicializar_matriz_led();

    sistema_ativo = false;
    selecionando  = true;
    temp_index = temp_count = 0;

    // I2C + OLED
    i2c_init(OLED_I2C_PORT, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&oled, 128, 64, false, OLED_ADDR, OLED_I2C_PORT);
    ssd1306_config(&oled);

    // PWM LED azul
    gpio_set_function(PIN_RGB_B, GPIO_FUNC_PWM);
    slice_b = pwm_gpio_to_slice_num(PIN_RGB_B);
    chan_b  = pwm_gpio_to_channel(PIN_RGB_B);
    pwm_set_wrap(slice_b, 65535);
    pwm_set_chan_level(slice_b, chan_b, 0);
    pwm_set_enabled(slice_b, true);

    // Criação das tasks
    xTaskCreate(Task_Sensor,    "Sensor",   256, NULL, 3, NULL);
    xTaskCreate(Task_Input,     "Input",    512, NULL, 2, NULL);
    xTaskCreate(Task_Control,   "Control",  512, NULL, 2, NULL);
    xTaskCreate(Task_Display,   "Display",  512, NULL, 1, NULL);
    xTaskCreate(Task_Buzzer,    "Buzzer",   256, NULL, 1, NULL);
    xTaskCreate(Task_Webserver, "WebSrv",  1280, NULL, 1, NULL);

    vTaskStartScheduler();
    while (true) tight_loop_contents();
    return 0;
}
