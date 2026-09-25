#pragma once

#include "esp_err.h"

namespace Cefet {

/**
 * @brief Analisador Sintatico e Construtor do Manifesto (JSON).
 *
 * Responsavel por desserializar o manifesto em texto plano recebido da rede,
 * alocar a arvore cJSON na memoria apenas uma vez, instanciar a lista de 
 * blocos funcionais via BlockRegistry e invocar o ConnectionManager para 
 * estabelecer o roteamento fisico.
 */
class JsonParser {
public:
    /**
     * @brief Analisa o manifesto, instancia os blocos e orquestra o roteamento.
     *
     * @param json_payload Ponteiro de caracteres com o manifesto.
     * @return esp_err_t ESP_OK se a malha foi completamente montada e roteada.
     */
    static esp_err_t parseManifest(const char* json_payload);
};

} // namespace Cefet