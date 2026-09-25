# 4deiacs - IEC 61499 ESP32 Runtime Node

O **4deiacs** é uma implementação de _Runtime Node_ baseada na norma **IEC 61499** para microcontroladores da família ESP32. Desenvolvido em C++ nativo utilizando o framework **ESP-IDF**, este projeto permite a criação e execução de sistemas de controle distribuídos orientados a eventos, totalmente configuráveis por arquivos JSON.

A arquitetura do projeto é altamente modularizada, encapsulando toda a lógica central e os blocos de função em um componente autônomo do ESP-IDF.

---

## 🛠 Requisitos

- **Hardware:** Placa de desenvolvimento **ESP32-S3** (O 4deiacs não é um firmware genérico e extrai o máximo desempenho especificamente do S3).
- **Software:** [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) configurado e instalado (preferencialmente versão 6.x).

---

## 🚀 Como Instalar e Configurar

O **4deiacs** não é um firmware genérico; ele extrai o máximo desempenho do ESP32-S3. Portanto, a configuração correta do hardware no ambiente de compilação é obrigatória.

1. **Clone o repositório** e entre na pasta do projeto.
2. **Dependências e Bibliotecas:**
   O projeto utiliza o _ESP Component Manager_ para resolver bibliotecas como Lua, MQTT, cJSON e mDNS. Quando baixadas pelo registry, elas vão para a pasta `managed_components/` (e não para a `components/`). Para registrar essas dependências manualmente, execute:
   ```bash
   idf.py add-dependency "espressif/lua^5.5.0"
   idf.py add-dependency "espressif/mqtt^1.0.0"
   idf.py add-dependency "espressif/cjson^1.7.19"
   idf.py add-dependency "espressif/mdns^1.11.0"
   ```
   _(Nota: Se o arquivo de manifesto já estiver presente, o `idf.py build` resolve e baixa tudo sozinho)._
3. **Configuração Fundamental (Menuconfig):**
   Abra o terminal e execute `idf.py menuconfig`. Faça as seguintes alterações obrigatórias:
   - **CPU & Clock:**
     - `Component config` ➔ `ESP System Settings` ➔ `CPU frequency`: Altere para **240 MHz**.
   - **Tempo Real (FreeRTOS):**
     - `Component config` ➔ `FreeRTOS` ➔ `Kernel` ➔ `configTICK_RATE_HZ`: Altere para **1000** (Garante a resolução de 1ms para a IEC 61499).
   - **Memória Externa (PSRAM):**
     - `Component config` ➔ `ESP PSRAM`: Ative **Support for external, SPI-connected RAM**.
     - Configure a velocidade para **80MHz** (Octal ou Quad Mode, dependendo da sua placa).
     - Ative a opção **Make RAM allocatable using malloc() as well** (Essencial para o parser JSON e a Máquina Virtual Lua).
   - **Particionamento e SPIFFS:**
     - `Serial flasher config` ➔ `Flash size`: Defina para o tamanho da sua placa (ex: **8 MB**).
     - `Partition Table` ➔ Altere para **Custom partition table CSV** e confirme se o arquivo está definido como `partitions.csv`.

4. **Credenciais de Rede e Provisionamento:**
   O Wi-Fi pode ser configurado via credenciais _hardcoded_ (para testes na planta) ou dinamicamente. O sistema utiliza **mDNS** para se anunciar na rede, permitindo que a Web IDE descubra o nó automaticamente e realize o provisionamento (renomeação) via _Hot-Deploy_.

5. **Gravação de Dados e Compilação:**
   O motor depende do sistema **SPIFFS** para persistir as malhas de controle e scripts Lua (manifestos JSON). Antes de compilar o firmware, grave os arquivos estáticos de configuração na memória Flash executando:
   ```bash
   idf.py spiffs-flash
   ```
   Em seguida, compile o código, grave na placa e abra o monitor serial com:
   ```bash
   idf.py build flash monitor
   ```

---

## 🧩 Catálogo de Blocos de Função (Function Blocks)

O sistema conta com um conjunto diversificado de blocos de função integrados que conectam a lógica de controle ao hardware do ESP32 e à rede.

### 📌 Entradas e Saídas Físicas (I/O)

- **Analog Input (`analog_input_block`)**: Realiza a leitura de sinais analógicos mapeados para os canais ADC do ESP32.
- **PWM Output (`pwm_output_block`)**: Gera sinais PWM genéricos nos pinos usando o periférico LEDC.
- **MCPWM Motor (`mcpwm_motor_block`)**: Bloco voltado para controle avançado de motores (ex: controle de velocidade e direção) aproveitando o hardware MCPWM.
- **Encoder Input (`encoder_input_block`)**: Efetua a contagem e leitura de encoders rotativos/quadratura usando o periférico PCNT, ideal para malhas de controle de velocidade ou posição.
- **Serial Monitor (`serial_monitor_block`)**: Exibe informações e dados dos portlets no console serial (útil para debug e monitoramento local).

### 🌐 Rede e Comunicação (Network)

- **MQTT Publisher / Subscriber (`mqtt_publisher_block`, `mqtt_subscriber_block`)**: Permitem a publicação e subscrição em tópicos de um broker MQTT. Indispensável para integração com plataformas de IoT em nuvem ou locais.
- **UDP Publisher / Subscriber (`udp_publisher_block`, `udp_subscriber_block`)**: Implementam a comunicação rápida e de baixa latência em rede local utilizando datagramas UDP.
- **Modbus TCP Server / Client (`modbus_tcp_server_block`, `modbus_tcp_client_block`)**: Permitem a interoperabilidade industrial com CLPs, supervisórios (SCADA) e inversores através do protocolo Modbus sobre TCP/IP.

### 🧮 Matemática e Lógica

- **Math Node (`math_node_block`)**: Permite a avaliação e resolução em tempo real de expressões matemáticas complexas (alimentado internamente pela biblioteca TinyExpr).
- **State Space (`state_space_block`)**: Realiza cálculos para simulação e modelagem de equações em espaço de estados (útil para aplicações avançadas de Teoria de Controle).
- **Sandbox (`sandbox_block`)**: Bloco dinâmico e flexível que executa lógica customizada por scripts (Lua), permitindo comportamentos que não estão pré-compilados.

### ⏱️ Temporizadores (Timers)

- **E-Cycle (`e_cycle_block`)**: Atua como o gerador de _ticks_ ou _clock_ cíclico. Ele dispara eventos ("EVENT_OUT") em intervalos regulares configurados, servindo de gatilho principal para os fluxos IEC 61499.

---

## 📂 Estrutura de Diretórios

A lógica principal reside no componente `4deiacs`. A pasta `main` atua exclusivamente como o ponto de entrada da aplicação.

- `main/`: Contém apenas o `main.cpp` que inicializa o componente.
- `components/4deiacs/`: O núcleo do sistema IEC 61499.
  - `include/` e `src/`: Cabeçalhos e códigos-fonte da Engine e dos Blocos de Função.

---

**Desenvolvido com o objetivo de aproximar normas de automação industrial (IEC 61499) da flexibilidade de microcontroladores modernos.**
