#pragma once

#include "esp_event.h"

namespace Cefet {

/**
 * @brief Declaração da Família de Eventos do motor CEFET-61499.
 * * Macro nativa do ESP-IDF. Define uma string base para que o barramento
 * diferencie os eventos do nosso sistema dos eventos do Wi-Fi ou Bluetooth.
 */
ESP_EVENT_DECLARE_BASE(CEFET_CORE_EVENTS);

/**
 * @brief Enumeração dos IDs de eventos internos do sistema.
 * * Estes IDs substituem as tradicionais "linhas de eventos" visuais da 
 * norma IEC 61499. Eles atuam como gatilhos (triggers) que acordam 
 * os blocos funcionais correspondentes, eliminando a necessidade de polling.
 */
enum EventIds {
    EV_SYSTEM_BOOT = 0,         /*!< Sistema inicializado e pronto para operar. */
    EV_SENSOR_DATA_READY,       /*!< Novo dado de hardware convertido pelo ADC/GPIO. */
    EV_NETWORK_CONNECTED,
    EV_CONTROL_CALC_DONE,       /*!< Algoritmo (ex: PID) finalizou seu cálculo matemático. */
    EV_NETWORK_RX,              /*!< Pacote de dados recebido pela rede (MQTT/Modbus/UDP). */
    EV_CONFIG_UPDATED           /*!< Novo arquivo JSON recebido, requisita reconfiguração a quente. */
};

} // namespace Cefet