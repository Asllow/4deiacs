#pragma once

#include <string>
#include "esp_err.h"

namespace Cefet {

/**
 * @brief Gerenciador do Sistema de Arquivos da Memoria Flash (SPIFFS).
 *
 * Responsavel por montar a particao de dados interna do ESP32,
 * prover metodos de leitura padrao e gerenciar a persistencia da malha
 * de controle dinamica (JSON) para hot-deploy e recuperacao de estado.
 */
class SpiffsManager {
public:
    /**
     * @brief Monta a particao SPIFFS no caminho virtual "/spiffs".
     *
     * @return esp_err_t ESP_OK em caso de sucesso.
     */
    static esp_err_t mount();

    /**
     * @brief Desmonta a particao SPIFFS.
     */
    static void unmount();

    /**
     * @brief Le o conteudo integral de um arquivo de texto generico.
     *
     * @param path Caminho absoluto do arquivo (ex: "/spiffs/config.json").
     * @return std::string Conteudo do arquivo (ou string vazia em caso de erro).
     */
    static std::string readFile(const std::string& path);

    /**
     * @brief Salva o manifesto da malha de controle na memoria nao-volatil.
     *
     * Sobrescreve o arquivo "/spiffs/mesh.json" garantindo que o dispositivo
     * carregue a malha correta no proximo ciclo de inicializacao.
     *
     * @param json_string Payload JSON contendo a definicao da malha.
     * @return esp_err_t ESP_OK em caso de sucesso, ESP_FAIL caso contrario.
     */
    static esp_err_t saveMesh(const std::string& json_string);

    /**
     * @brief Le o manifesto da malha persistida alocando diretamente na PSRAM.
     *
     * @warning O ponteiro retornado deve ser desalocado pelo chamador utilizando
     * heap_caps_free() para evitar vazamento de memoria externa.
     *
     * @return char* Ponteiro para a string C terminada em nulo contendo o JSON,
     * ou nullptr caso o arquivo nao exista.
     */
    static char* readMesh();
};

} // namespace Cefet