# 🚀 ThermoGuard – Sistema de Controle de Temperatura Inteligente com Interface Web
> *Um sistema completo para monitoramento e controle de temperatura utilizando Raspberry Pi Pico W, com display OLED, feedback tátil e sonoro, e uma interface web interativa.*

## 📝 Descrição Breve
Este projeto implementa um sistema de controle de temperatura multifuncional baseado no microcontrolador Raspberry Pi Pico W. Ele lê dados de temperatura e umidade de um sensor DHT11, permite ao usuário definir um setpoint de temperatura através de um joystick e um botão, ou remotamente via uma interface web. O sistema utiliza um controlador Proporcional-Integral (PI) para ajustar um atuador (simulado por um LED PWM e um valor de RPM) com o objetivo de manter a temperatura ambiente próxima ao setpoint. O feedback visual é fornecido por um display OLED SSD1306, que exibe dados em tempo real (temperatura, setpoint, erro, PWM, RPM), e uma matriz de LED 8x8 que indica a magnitude do erro. Um buzzer fornece alertas sonoros para desvios significativos de temperatura. A conectividade Wi-Fi é gerenciada pelo chip CYW43_arch do Pico W, permitindo acesso a um dashboard web para monitoramento e controle. O projeto é construído sobre FreeRTOS para gerenciamento eficiente de tarefas concorrentes.

**Componentes principais envolvidos:**
*   Raspberry Pi Pico W
*   Sensor de Temperatura e Umidade DHT11
*   Display OLED I2C SSD1306 (128x64)
*   Joystick Analógico (para entrada de setpoint)
*   Botão Táctil (para confirmação e modo)
*   LED (para indicar a saída do controle PWM)
*   Buzzer Passivo (para alertas sonoros)
*   Matriz de LED 8x8 (com driver MAX7219, para visualização de erro)

**Uso esperado ou aplicação prática:**
*   Protótipo para sistemas de controle de clima (HVAC).
*   Ferramenta educacional para aprender sobre sistemas embarcados, IoT, controle PI e FreeRTOS.
*   Base para projetos de automação residencial ou estufas inteligentes.

**Tecnologias ou bibliotecas utilizadas:**
*   Linguagem C
*   Raspberry Pi Pico SDK
*   FreeRTOS (Real-Time Operating System)
*   LwIP (Lightweight IP stack para TCP/IP)
*   Bibliotecas customizadas para DHT11, SSD1306 e Matriz de LED.

## ✨ Funcionalidades Principais
*   📈 **Leitura Precisa de Sensores:** Coleta de dados de temperatura e umidade do ambiente utilizando o sensor DHT11.
*   🕹️ **Entrada de Usuário Intuitiva:** Ajuste do setpoint de temperatura via joystick e botão tátil.
*   🧠 **Controle PI Inteligente:** Implementação de um controlador Proporcional-Integral (PI) para regular a temperatura.
*   💡 **Atuação PWM:** Controle de um LED via PWM, simulando a potência aplicada a um aquecedor/resfriador e indicando RPM de um motor virtual.
*   🖥️ **Display OLED Informativo:** Exibição em tempo real de temperatura atual, setpoint, erro, valor PWM, RPM simulado e status do sistema.
*   📊 **Visualização de Erro em Matriz de LED:** Indicação da magnitude do erro de temperatura (diferença entre setpoint e real) em uma matriz de LED 8x8.
*   🔔 **Alertas Sonoros:** Buzzer para notificar o usuário sobre desvios críticos ou significativos da temperatura desejada.
*   🌐 **Interface Web Responsiva:** Dashboard web acessível via Wi-Fi para monitoramento remoto e ajuste do setpoint, com atualização automática.
*   🔄 **Multitarefa com FreeRTOS:** Gerenciamento eficiente de múltiplas operações (leitura de sensor, entrada de usuário, controle, atualização de display, servidor web) de forma concorrente.
*   📡 **Conectividade Wi-Fi:** Utilização do módulo Wi-Fi do Pico W para comunicação em rede local.

## ⚙️ Pré-requisitos / Hardware Necessário
### Hardware
| Componente                        | Quant. | Observações                                                                 |
| :-------------------------------- | :----: | :-------------------------------------------------------------------------- |
| Raspberry Pi Pico W               |   1    | Com headers soldados é recomendado.                                         |
| Sensor de Temperatura DHT11       |   1    | Módulo com resistor de pull-up embutido é mais fácil de usar.               |
| Display OLED I2C SSD1306 128x64   |   1    | Monocromático. Verifique a tensão (geralmente 3.3V/5V).                     |
| Joystick Analógico (KY-023)       |   1    | Apenas o eixo Y é utilizado neste projeto (ADC).                             |
| Botão Táctil (Push Button)        |   1    | Com resistor de pull-up externo ou usando o interno do Pico.                |
| LED (ex: Azul, 5mm)               |   1    | Usado para simular o atuador PWM. Requer resistor limitador de corrente.    |
| Resistor (para LED)               |   1    | Ex: 220Ω ou 330Ω para LED conectado a 3.3V.                               |
| Buzzer Passivo                    |   1    | Controlado por PWM para diferentes tons/alertas.                          |
| Matriz de LED 8x8 com MAX7219     |   1    | Módulo SPI.                                                                 |
| Protoboard                        |   1    | Para montagem do circuito.                                                  |
| Jumpers Macho-Macho e Macho-Fêmea | Vários | Para conexões.                                                              |
| Cabo Micro USB                    |   1    | Para alimentação e programação do Pico W.                                   |

### Software / Ferramentas
*   **Raspberry Pi Pico SDK:** Versão mais recente recomendada (testado com v1.5.1).
*   **ARM GCC Toolchain:** (e.g., `arm-none-eabi-gcc` versão 10.3 ou superior).
*   **CMake:** Versão 3.13 ou superior.
*   **Git:** Para clonar o repositório e seus submódulos.
*   **Visual Studio Code (Opcional):** Com extensões C/C++ e CMake Tools para facilitar o desenvolvimento.
*   **Sistema Operacional Testado:** Linux (Ubuntu 22.04), macOS (Ventura), Windows 10/11 (com WSL2 ou Pico Toolchain).
*   **Terminal Serial:** PuTTY, minicom, Tera Term, ou o terminal integrado do VS Code (baud rate 115200).
*   **Navegador Web Moderno:** Chrome, Firefox, Edge, Safari para acessar a interface web.

## 🔌 Conexões / Configuração Inicial
### Pinagem resumida
| Pino Pico (GP) | Componente        | Função/Conexão                                            |
| :------------- | :---------------- | :-------------------------------------------------------- |
| GP16           | Sensor DHT11      | Pino de Dados                                             |
| GP26 (ADC0)    | Joystick          | Eixo Y (VRy)                                              |
| GP5            | Botão A           | Sinal do Botão (Pull-up interno ou externo para 3.3V)     |
| GP12           | LED Azul          | Anodo do LED (Cátodo para GND via resistor) - PWM         |
| GP10           | Buzzer            | Terminal positivo do Buzzer (outro terminal para GND) - PWM |
| GP14 (I2C1 SDA)| Display OLED      | SDA                                                       |
| GP15 (I2C1 SCL)| Display OLED      | SCL                                                       |
| **Matriz LED** |                   |                                                           |
| GP17 (SPI0 CS) | Matriz LED MAX7219| CS (Chip Select)                                          |
| GP18 (SPI0 SCK)| Matriz LED MAX7219| CLK (Clock)                                               |
| GP19 (SPI0 TX) | Matriz LED MAX7219| DIN (Data In)                                             |
| 3V3 (OUT)      | Vários            | Alimentação 3.3V para sensores e display                  |
| GND            | Vários            | Referência comum de terra para todos os componentes       |

> **Nota Importante:**
> *   Certifique-se de que todos os componentes compartilham um **GND comum** com o Raspberry Pi Pico W.
> *   A tensão de alimentação para os periféricos (DHT11, OLED, Joystick) deve ser de **3.3V**, fornecida pelo pino `3V3 (OUT)` do Pico.
> *   Verifique a pinagem específica da sua biblioteca `matriz_led.h` para a Matriz de LED, especialmente os pinos SPI (CS, SCK, DIN). O exemplo acima usa SPI0 com GP17, GP18, GP19. Ajuste conforme necessário.
> *   Não se esqueça do resistor limitador de corrente para o LED.

### Configuração de Software (primeira vez)
1.  **Clone o repositório:**
    ```bash
    git clone https://github.com/SEU_USUARIO/Nome-do-Projeto.git
    cd Nome-do-Projeto
    ```

2.  **Inicialize e atualize os submódulos (Pico SDK e outras bibliotecas):**
    O Pico SDK é frequentemente incluído como um submódulo.
    ```bash
    git submodule update --init --recursive
    ```
    Se o SDK não for um submódulo, certifique-se de que ele esteja clonado e a variável `PICO_SDK_PATH` esteja configurada corretamente (veja a seção de compilação).

3.  **Configure as credenciais Wi-Fi:**
    Abra o arquivo `main.c` e edite as seguintes macros com os dados da sua rede Wi-Fi:
    ```c
    //Configurações de Wi-Fi
    #define WIFI_NOME_REDE "SEU_SSID_AQUI"
    #define WIFI_SENHA     "SUA_SENHA_AQUI"
    ```

## ▶️ Como Compilar e Executar
Siga estes passos para compilar o projeto usando CMake e Make:

1.  **Crie e acesse o diretório de build:**
    A partir da raiz do projeto (`Nome-do-Projeto`):
    ```bash
    mkdir build
    cd build
    ```

2.  **Configure o caminho para o Pico SDK (se não estiver configurado globalmente ou se não for um submódulo no local padrão):**
    Ajuste o caminho conforme a localização do seu SDK. Se o SDK está em `Nome-do-Projeto/pico-sdk`, o caminho relativo seria `../../pico-sdk`.
    ```bash
    export PICO_SDK_PATH=/caminho/absoluto/para/pico-sdk
    # ou, se relativo ao diretório 'build':
    # export PICO_SDK_PATH=../../pico-sdk
    ```

3.  **Execute o CMake para gerar os Makefiles:**
    ```bash
    cmake ..
    ```

4.  **Compile o projeto:**
    Use `-jN` para compilação paralela, onde `N` é o número de núcleos do seu processador (ex: `-j4` ou `-j$(nproc)` no Linux).
    ```bash
    make -j$(nproc)
    # ou
    # make -j4
    ```
    Isso gerará um arquivo `.uf2` (e.g., `Nome-do-Projeto.uf2`) dentro do diretório `build`.

**Para subir para a placa (Raspberry Pi Pico W):**
1.  Desconecte o Pico W da alimentação (USB).
2.  Pressione e mantenha pressionado o botão **BOOTSEL** no Pico W.
3.  Conecte o Pico W ao seu computador via cabo USB enquanto mantém o BOOTSEL pressionado.
4.  O Pico W aparecerá como um dispositivo de armazenamento em massa (como um pen drive).
5.  Arraste e solte o arquivo `.uf2` gerado (e.g., `Nome-do-Projeto.uf2` de dentro da pasta `build`) para dentro do dispositivo de armazenamento do Pico W.
6.  O Pico W irá reiniciar automaticamente e começar a executar o firmware.

**Como acessar logs/dashboards:**
*   **Logs (Serial):**
    *   Conecte-se ao Pico W usando um programa de terminal serial (PuTTY, minicom, Tera Term, etc.).
    *   Configure a porta serial correspondente ao Pico e use uma taxa de transmissão (baud rate) de **115200 bps**.
    *   Você verá mensagens de inicialização, status da conexão Wi-Fi (incluindo o endereço IP) e outros logs de depuração.
*   **Dashboard Web:**
    *   Após o Pico W conectar-se à sua rede Wi-Fi, o endereço IP será exibido no terminal serial.
    *   Abra um navegador web no mesmo dispositivo da rede e digite o endereço IP do Pico W (e.g., `http://192.168.1.XX`).
    *   A interface web do ThermoController será carregada, permitindo monitoramento e controle.

### 📁 Estrutura do Projeto

Nome-do-Projeto/
├── build/ # Diretório de compilação (gerado)
├── lib/ # Bibliotecas de terceiros ou customizadas
│ ├── Display_Bibliotecas/
│ │ ├── ssd1306.c
│ │ └── ssd1306.h
│ ├── dht11/
│ │ ├── dht11.c
│ │ └── dht11.h
│ └── Matriz_Bibliotecas/
│ ├── matriz_led.c
│ └── matriz_led.h
├── main.c # Arquivo principal com a lógica da aplicação
├── CMakeLists.txt # Script de configuração do CMake para o projeto
├── pico_sdk_import.cmake # Script do CMake para importar o Pico SDK
├── pico-sdk/ # Submódulo: Raspberry Pi Pico SDK (se incluído assim)
└── README.md # Este arquivo

*   **`main.c`**: Contém toda a lógica principal da aplicação, incluindo inicialização de hardware, definições de tasks do FreeRTOS (leitura de sensor, entrada de usuário, controle PI, atualização de display, buzzer, servidor web) e a função `main()`.
*   **`lib/`**: Agrupa bibliotecas de hardware específicas.
    *   **`Display_Bibliotecas/`**: Código para controle do display OLED SSD1306.
    *   **`dht11/`**: Código para interface com o sensor de temperatura e umidade DHT11.
    *   **`Matriz_Bibliotecas/`**: Código para controle da matriz de LED 8x8.
*   **`CMakeLists.txt`**: Define como o projeto é compilado, incluindo fontes, bibliotecas e dependências.
*   **`pico_sdk_import.cmake`**: Script padrão do Pico SDK para facilitar a inclusão do SDK no processo de build do CMake.
*   **`pico-sdk/` (opcional, se como submódulo)**: Cópia local do Raspberry Pi Pico SDK.
*   **`build/`**: Diretório onde os arquivos de compilação (objetos, executáveis `.elf`, `.uf2`) são armazenados. Este diretório é geralmente ignorado pelo Git.

## 👤 Autor / Contato
*   **Nome:** Jonas Souza 
*   **E-mail:** Jonassouza871@hotmail.com
