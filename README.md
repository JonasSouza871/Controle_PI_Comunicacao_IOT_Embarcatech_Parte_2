# 🚀 ThermoGuardian – Sistema de Monitoramento & Controle de Temperatura 🚀
> *“Monitore, controle e visualize em tempo-real a temperatura com um Raspberry Pi Pico W, display OLED e interface web.”*

## 📝 Descrição Breve
Este projeto implementa um sistema embarcado completo que **lê a temperatura** de um sensor *DS18B20*, aplica um **controle PI** para acionar um **LED RGB** (via PWM) e um **buzzer**, exibe informações em um **display OLED SSD1306** e em uma **matriz WS2812**, além de publicar os dados em um **servidor web** integrado ao Pico W (interface auto-atualizada a cada 2 s).  
Ideal para laboratórios, protótipos de automação residencial ou demonstrações acadêmicas de sistemas de controle em tempo real usando FreeRTOS.

## ✨ Funcionalidades Principais
* 📈 **Aquisição de temperatura** (DS18B20) a cada 1 s  
* 🎛️ **Ajuste de *setpoint*** com joystick analógico + botão  
* 🔄 **Controle PI** (C, tempo real, FreeRTOS)  
* 🌈 **Feedback visual** – LED RGB proporcional ao erro & animação na matriz LED  
* 🔔 **Alarmes sonoros** – Buzzer com frequências/timers diferentes conforme o erro  
* 🖥️ **Display OLED** (menus de ajuste, dashboards, barras de progresso)  
* 🌐 **Webserver HTTP** na porta 80 com página auto-refresh (HTML puro)  
* 🕘 **Simulação de RPM** derivada do duty-cycle de PWM  
* ⚙️ Estrutura modular em C, organizada com Pico SDK, FreeRTOS e CMake

## ⚙️ Pré-requisitos / Hardware Necessário
### Hardware
| Componente | Quant. | Observações |
|------------|--------|-------------|
| Raspberry Pi **Pico W** | 1 | MCU RP2040 + Wi-Fi |
| Sensor **DS18B20** | 1 | Pino 20 (1-Wire) |
| **OLED SSD1306** 128×64 (I²C) | 1 | SDA 14 / SCL 15 |
| **Matriz WS2812** (8×8 ou equivalente) | 1 | Conecte ao pino definido em `matriz_led.c` |
| **LED RGB** comum (anodo comum) | 1 | R-11 / G-12 / B-13 (PWM) |
| **Joystick analógico** | 1 | Eixo Y no ADC0 (pino 26) + botão em 5 |
| **Buzzer piezo** | 1 | Pino 10 (PWM) |
| Fonte 5 V ≥ 1 A | 1 | Ou USB do computador |

### Software / Ferramentas
* **Pico SDK** ≥ 1.5 + **pico-extras**  
* **FreeRTOS-Kernel** (já incluso na pasta `lib/`)  
* **CMake** ≥ 3.13 & **ARM GCC** (ou `pico-sdk` toolchain)  
* **Python 3** (apenas para ferramentas de upload UF2 – opcional)  
* OS testado: **Linux Mint / Ubuntu** (funciona também em Windows via *PicoToolchain* ou WSL)

## 🔌 Conexões / Configuração Inicial
### Pinagem resumida
- DS18B20 -> GP20
- Joystick VRY -> GP26 (ADC0)
- Botão A -> GP5 (entrada pull-up)
- LED RGB R/G/B -> GP11 / GP12 / GP13 (PWM)
- Buzzer -> GP10 (PWM)
- OLED SDA/SCL -> GP14 / GP15 (I²C1 400 kHz)

> Tenha GND comum e 3 V3 para os dispositivos; WS2812 requer 5 V (ou 3 V3 com nível adequado).

### Configuração de Software (primeira vez)
```bash
# 1) Clone o repositório
git clone https://github.com/<seu-usuario>/controle-picoW.git
cd controle-picoW

# 2) Inicialize submódulos (FreeRTOS / libs)
git submodule update --init --recursive

Edite Wifi/wifi_config.h (ou as macros no main.c) com SSID e PASSWORD da sua rede.

```
### ▶️ Como Compilar e Executar

```bash
# Diretório raiz do projeto
mkdir build && cd build
export PICO_SDK_PATH=/caminho/para/pico-sdk      # ajuste conforme seu ambiente
cmake ..
make -j4                                         # gera .uf2
# Pico em modo BOOTSEL → copie arquivo .uf2 gerado para a unidade montada
```

Depois de flash:

- Abra um monitor serial (115200 baud) para logs.

- Acesse http://<IP_do_PicoW> no navegador para visualizar o dashboard (auto-refresh).


### 📁 Estrutura do Projeto:

```bash
.
├── lib/
│   ├── Display_Bibliotecas/
│   │   ├── ssd1306.c/.h
│   │   └── font.h
│   ├── DS18b20/
│   ├── Matriz_Bibliotecas/
│   │   ├── matriz_led.c/.h
│   │   └── ws2812.pio
│   └── Wifi/
│       ├── lwipopts.h
│       └── wifi_config.h
├── FreeRTOSConfig.h          # Ajustes do kernel
├── main.c                    # **Ponto de entrada** com todas as tasks
├── CMakeLists.txt            # Script de build
└── README.md                 # Este arquivo

```

## 🐛 Debugging / Solução de Problemas

* **Sem Wi-Fi?** Verifique **SSID/senha** e a proximidade do roteador.  
* **OLED apagado?** Confirme o endereço **I²C (`0x3C`)** e a solda dos jumpers.  
* **WS2812 piscando estranho?** Use fonte **5 V dedicada** ou aumente o resistor de série (≈ 330 Ω).  
* **FreeRTOS travando?** Aumente o **stack size** das tasks no `xTaskCreate`.  
* **Página web sem atualizar?** Cheque os **logs LWIP** e o **firewall** da rede.

## 👤 Autor / Contato


 **Autor:**  Jonas Souza           
 **E-mail:** Jonassouza871@hotmail.co         
