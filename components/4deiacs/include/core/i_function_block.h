#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "4deiacs_node_engine.h"

namespace deiacs {

/**
 * @brief Base interface for all Function Blocks in the 4deiacs framework.
 * 
 * This interface guarantees system polymorphism. Any control block, 
 * hardware I/O, or network interface must inherit from this class. This allows 
 * the Instance Manager (JSON Parser) to treat all blocks generically, 
 * allocating them dynamically in the ESP32 RAM.
 */
class IFunctionBlock {
protected:
    /**
     * @brief Internal structure to map the destination of an event.
     */
    struct EventTarget {
        IFunctionBlock* block;
        std::string port;
    };

    /** @brief Map of output event routes */
    std::unordered_map<std::string, std::vector<EventTarget>> m_event_routes;

    /**
     * @brief Triggers an Output Event (Event Out).
     * Enqueues the event execution for all blocks connected to this port.
     * 
     * @param event_out_name Port name (e.g., "CNF" or "EV_OUT").
     */
    void emitEvent(const std::string& event_out_name) {
        auto it = m_event_routes.find(event_out_name);
        if (it != m_event_routes.end()) {
            for (auto& target : it->second) {
                if (target.block) {
                    DeiacsEngine::enqueueBlockEvent(target.block, target.port);
                }
            }
        }
    }

public:
    /**
     * @brief Default virtual destructor.
     * Ensures proper memory deallocation for derived classes.
     */
    virtual ~IFunctionBlock() = default;

    /**
     * @brief Function Block initialization routine.
     * Must be called right after instantiation to configure peripherals
     * (e.g., GPIO pins) or allocate network resources.
     * 
     * @return true If successfully initialized.
     * @return false If an error occurred (e.g., hardware unresponsive).
     */
    virtual bool initialize() = 0;

    /**
     * @brief Retrieves the unique identifier of the instantiated block.
     * This ID is used to create the connections (Wiring) between blocks.
     * 
     * @return std::string containing the ID (e.g., "TANK_LEVEL_PID_1").
     */
    virtual std::string getId() const = 0;

    // =========================================================================
    // IEC 61499 ROUTING PORTS
    // =========================================================================

    /**
     * @brief Retrieves the memory pointer of a Data Output port (Data Out).
     * 
     * @param port_name Name of the port according to the standard (e.g., "DATA_OUT").
     * @return void* Pointer to the internal variable of the block, or nullptr if it does not exist.
     */
    virtual void* getDataOutput(const std::string& port_name) {
        return nullptr; 
    }

    /**
     * @brief Connects an external pointer to a Data Input port (Data In).
     * The block will read data directly from this memory region.
     * 
     * @param port_name Name of the port according to the standard (e.g., "PAYLOAD_IN").
     * @param data_pointer Memory pointer originating from another block.
     * @return true If the port exists and the connection was accepted.
     */
    virtual bool connectDataInput(const std::string& port_name, void* data_pointer) {
        return false;
    }

    /**
     * @brief Triggers an Event Input port (Event In).
     * Executes the block's internal logic associated with this event.
     * 
     * @param event_name Name of the event according to the standard (e.g., "REQ" or "INIT").
     */
    virtual void triggerEventInput(const std::string& event_name) {
    }

    /**
     * @brief Registers an event wire linking the output of this block to the input of another.
     * 
     * @param event_out_name Name of the output port of this block (e.g., "CNF").
     * @param target_block Pointer to the target block instance in memory.
     * @param target_port Name of the input port on the target block (e.g., "REQ").
     */
    virtual void connectEventOutput(const std::string& event_out_name, IFunctionBlock* target_block, const std::string& target_port) {
        m_event_routes[event_out_name].push_back({target_block, target_port});
    }
};

} // namespace deiacs