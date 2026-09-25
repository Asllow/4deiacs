/**
 * @file analog_input_block.h
 * @brief Servico de Entrada Analogica (CSIFB) com tipagem padronizada.
 */
#pragma once

#include <string>
#include "i_function_block.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "cJSON.h"

namespace Cefet {

/**
 * @brief Analog Input Service Interface Function Block (SIFB).
 *
 * Encapsula o driver ADC do ESP-IDF v6.0.
 * Converte o valor bruto lido para float, garantindo o polimorfismo
 * e a integridade dos ponteiros na troca de dados com outros blocos 
 * matematicos e de rede.
 */
class AnalogInputBlock : public IFunctionBlock {
public:
    /**
     * @brief Instancia o Bloco de Entrada Analogica.
     *
     * @param block_id Identificador unico na rede.
     * @param adc_unit Unidade de hardware ADC.
     * @param adc_channel Canal ADC correspondente ao GPIO fisico.
     */
    AnalogInputBlock(const std::string& block_id, adc_unit_t adc_unit, adc_channel_t adc_channel);

    /**
     * @brief Destrutor. Liberta os recursos de hardware do ADC.
     */
    ~AnalogInputBlock() override;

    /**
     * @brief Inicializa e calibra o periferico no ESP32.
     *
     * @return true se a alocacao for bem-sucedida.
     */
    bool initialize() override;

    /**
     * @brief Recupera a identificacao do bloco.
     *
     * @return std::string O ID configurado.
     */
    std::string getId() const override;

    /**
     * @brief Realiza a conversao analogico-digital bruta.
     *
     * @param out_value Ponteiro para armazenar o resultado inteiro do hardware.
     * @return true se a leitura for bem-sucedida.
     */
    bool readRaw(int* out_value);

    /* PORTAS IEC 61499 */

    /**
     * @brief Expoe o endereco de memoria da variavel tipada em float.
     */
    void* getDataOutput(const std::string& port_name) override;

    /**
     * @brief Processa eventos de entrada (Ex: REQ).
     */
    void triggerEventInput(const std::string& event_name) override;

    /**
     * @brief Factory method para instanciacao via JSON.
     */
    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    adc_unit_t m_unit;
    adc_channel_t m_channel;
    adc_oneshot_unit_handle_t m_adc_handle;
    bool m_initialized;

    /** * @brief Variavel interna tipada em float para seguranca de ponteiros.
     * Substitui o int original que causava corrupcao de memoria ao ser 
     * lido por blocos matematicos.
     */
    float m_data_out; 
};

} /* namespace Cefet */