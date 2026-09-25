/**
 * @file pwm_output_block.h
 * @brief Bloco de Funcao de Interface com Hardware (Hardware Interface Function Block) para geracao de PWM.
 * Adequado aos principios SOLID, atuando de forma desacoplada do motor de eventos.
 */
#pragma once

#include <string>
#include <vector>
#include "driver/ledc.h"
#include "i_function_block.h"
#include "cJSON.h"

namespace Cefet {

/**
 * @class PwmOutputBlock
 * @brief Encapsula o periferico LEDC do ESP-IDF para geracao de sinais PWM padronizados.
 */
class PwmOutputBlock : public IFunctionBlock {
public:
    /**
     * @brief Construtor padrao.
     * * @param block_id Identificador unico na rede IEC 61499.
     * @param gpio_num Pino de saida de hardware.
     * @param timer_num Instancia do timer LEDC associado.
     * @param channel_num Canal LEDC alocado.
     * @param freq_hz Frequencia de operacao em Hertz.
     */
    PwmOutputBlock(const std::string& block_id, int gpio_num, ledc_timer_t timer_num, ledc_channel_t channel_num, uint32_t freq_hz);

    /**
     * @brief Destrutor padrao que interrompe o hardware em seguranca.
     */
    ~PwmOutputBlock() override;

    bool initialize() override;
    std::string getId() const override;
    
    /**
     * @brief Conecta um ponteiro de memoria externa a uma porta de entrada de dados generica.
     * * @param port_name Nome da porta (ex: "DUTY_CYCLE").
     * @param data_pointer Ponteiro para a variavel na memoria heap.
     * @return true se a porta existir e for acoplada com sucesso.
     */
    bool connectDataInput(const std::string& port_name, void* data_pointer) override;

    /**
     * @brief Gatilho polimorfico para execucao baseada em eventos.
     * * @param event_name Nome do evento de entrada (ex: "REQ").
     */
    void triggerEventInput(const std::string& event_name) override;

    static IFunctionBlock* create(const std::string& block_id, cJSON* config);

private:
    std::string m_id;
    int m_gpio_num;
    ledc_timer_t m_timer_num;
    ledc_channel_t m_channel_num;
    uint32_t m_freq_hz;
    std::vector<float*> m_inputs;

    /**
     * @brief Modifica em baixo nivel a razao ciclica do PWM.
     * * @param duty_cycle Valor bruto do duty cycle (0-8191 para 13-bit).
     * @return true em caso de sucesso no barramento.
     */
    bool writePwm(uint32_t duty_cycle);
};

} // namespace Cefet