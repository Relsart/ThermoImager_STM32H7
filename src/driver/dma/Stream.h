#pragma once

#include <stdint.h>
#include "etl/atomic.h"

#include "stm32h7xx.h"
#include "driver/nvic/Nvic.h"
#include "DataTypes.h"
#include "Signal.h"
#include "Dma.h"

namespace driver {
namespace dma {

const uint32_t maxStreamTransactionSize = 0xFFFF;   // Counter of stream transceiving data has 16 bit

class Stream : public SlotInterface<Irq>
{
private:
    __IO DMA_Stream_TypeDef* const m_stream;    // Stream periphery pointer
    uint32_t* m_ifcr;                       // Clearing interruption flags register (LIFCR or HIFCR: depending on the stream number)
    uint32_t* m_isr;                        // Interruption state register (LISR or HISR: depending on the stream number)
    DMAMUX_Channel_TypeDef* m_mux;          // DMA Multiplexor channnel
    uint8_t m_muxNumber;                    // DMAMUX channel number (0...15)
    StreamConfig m_config;                  // Saved configuration
    Signal<SignalPack> m_transComplete;     // Signal of transfer data complete
    uint16_t m_dataSize = 0;                // Size of exchanged data (saved from transfer request)

    etl::atomic<bool> m_busy{false};        // Busy stream flag
    bool m_initOk = false;                  // Stream initialised flag
    bool m_errorFlag = false;               // Stream error-state flag

    /**
     * @brief Bits positions for ISR/IFCR registers
     */
    struct IrPos
    {
        uint32_t mask;  // All Ir bits mask for current stream
        uint8_t tcif;   // Transfer Complete Interrupt Flag position
        uint8_t htif;   // Half Transfer complete Interrupt Flag position
        uint8_t teif;   // Transfer Error Interrupt Flag position
        uint8_t dmeif;  // Direct Mode Error Interrupt Flag position
        uint8_t feif;   // FIFO Error Interrupt Flag position
    } m_irPos{0};

    /**
     * @brief DMA Interruptions handler
     */
    void run(Irq, uint32_t) override;

    /**
     * @brief Stop stream and reset interruptions flags
     */
    void stopStream();

    /**
     * @brief Initiate the data transfer
     * @param [in] memaddr1 For periphery exchenge - buffer1 addr; for Mem2mem - address of data source
     * @param [in] memaddr2 For periphery exchenge - buffer2 addr (double-buffered mode); for Mem2mem - address of data destination
     * @param [in] dataSize Transceiving data size (in units). Max 0xFFFF
     * @param [in] unitSize Size of data unit (1/2/4 bytes)
     * @param [in] memInc If false: disabling memory increment- for sending the same data several times
     */
    void transferData (uint32_t memaddr1, uint32_t memaddr2, uint16_t dataSize, uint8_t unitSize, bool memInc = true);

    /**
     * @brief FIFO parameters checking for correctness
     * @param [in] config configurations data
     * @return True = Ok
     */
    bool checkFifoParams(const StreamConfig* configs);

    /**
     * @brief Check the memory address availability for DMA1, DMA2
     * @param [in] addr Address
     * @return True = Ok
     */
    bool checkMemAddr(uint32_t addr);

    /**
     * @brief Set periphery address (if it's not Mem2Mem), transmission direction and DMAMUX ID
     * @param [in] requestId DMAMUX multiplexor request Id
     * @return True = Ok
     */
    bool setPeriphAddr(MuxRequestIDs requestId);

public:
    Nvic* nvic; // NVIC instance pointer for priority management
    
    /**
     * @brief Constructor
     */
    Stream();

    /**
     * @brief Set stream configurations
     * @param [in] config configurations data
     * @return True = Ok
     */
    bool configurate(const StreamConfig* config);

    /**
     * @brief Get stream configurations
     */
    StreamConfig getConfig();

    /**
     * @brief Stream re-initialization with previously set (in configurate()) configurations
     */
    bool reInit();

    /**
     * @brief Data transaction between memory and periphery (for General or Circular Single-Buffered mode)
     * @details Periphery address and transfer direction has allready set in configurate method
     * @param [in] memAddr Memory address
     * @param [in] dataSize size of data (in units of increment)
     * @param [in] incSize increment (one unit) size in bytes
     * @return Result of transfer checking. True = Ok
     */
    bool periphTransfer(void* memAddr, uint16_t dataSize, uint8_t incSize, bool memInc = true);

    /**
     * @brief Data transaction between memory and periphery (for Circular Double-Buffered mode)
     * @details Periphery address and transfer direction has allready set in configurate method
     * @param [in] memBuff1 Memory buffer 1 address
     * @param [in] memBuff2 Memory buffer 2 address
     * @param [in] dataSize size of data (in units of increment)
     * @param [in] incSize increment (one unit) size in bytes
     * @return Result of transfer checking. True = Ok
     */
    bool periphTransfer(void* memBuff1, void* memBuff2, uint16_t dataSize, uint8_t incSize);


    /**
     * @brief Memory-to-Memory data transaction. Overloaded for bytes, halfwords and words  
     * @return Result of transfer checking. True = Ok
     */
    bool mem2memTransfer(uint8_t* sourceAddr, uint8_t* destAddr, uint16_t dataSize);
    bool mem2memTransfer(uint16_t* sourceAddr, uint8_t* destAddr, uint16_t dataSize);
    bool mem2memTransfer(uint32_t* sourceAddr, uint8_t* destAddr, uint16_t dataSize);

    /**
     * @brief Connect slot to Transfer Complete Interrupt signal
     */
    void onTransferCompleteSlot(SlotInterface<SignalPack>*slot);

    /**
     * @brief Flags getters
     */
    bool isInitialised();
    bool isBusy();
    bool isErrorState();
};

}   // namespace dma
}   // namespace driver
