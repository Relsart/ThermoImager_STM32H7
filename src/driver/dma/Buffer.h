#pragma once

#include <cstdint>
#include <cstddef>
#include "stm32h7xx.h"
#include "Interface.h"

namespace driver {
namespace dma {

template <size_t BufferSize>
class Buffer : public i_face::I_DmaBuffer
{
private:
    static constexpr size_t CacheLineSize = 32;
    static_assert(BufferSize % CacheLineSize == 0, "Error: DmaBuffer size must be a multiple of 32 bytes (Cache Line)!");

    alignas(CacheLineSize) uint8_t m_buffer[BufferSize];   // Buffer, aligned in SRAM by 32 bytes

    /**
     * @brief Check the memory address availability for DMA1, DMA2
     * @param [in] addr Address
     * @return True = Ok
     */
    static inline bool isAddrValidForDma(uint32_t addr)
    {
        // AXI SRAM (D1 Domain) — main gegion for high-speed DMA buffers:
        if (addr >= 0x24000000 && addr <= 0x2407FFFF) return true;
        // SRAM1, SRAM2, SRAM3 (D2 Domain) or SRAM4 (D3 Domain):
        if (addr >= 0x30000000 && addr <= 0x3800FFFF) return true;
        // Backup SRAM:
        if (addr >= 0x38800000 && addr <= 0x38800FFF) return true;
        // FMC (External SDRAM):
        if (addr >= 0x60000000 && addr <= 0x8FFFFFFF) return true;
        // QSPI / OCTOSPI (external Flash in Memory-Mapped mode):
        if (addr >= 0x90000000 && addr <= 0x9FFFFFFF) return true;
        // 6. Internal Flash memory banks 1, 2:
        if (addr >= 0x08000000 && addr <= 0x081FFFFF) return true;
        // DTCM (0x20000000) and ITCM (0x00000000) are not supported by DMA1/2!
        return false;
    }

public:
    /**
     * @brief Constructor
     */
    Buffer()
    {
        // Check, if buffer address in RAM supported by DMA1/2 and stuck here, if not:
        if (!isAddrValidForDma(reinterpret_cast<uint32_t>(m_buffer)))
        {
            // Stuck here:
            __asm volatile("bkpt #0"); 
            while(1);
        }
    }

    /**
     * @brief Get buffer address 
     * @details For SCB_InvalidateDCache_by_Addr() or SCB_CleanDCache_by_Addr() functions
     */
    uint32_t* getAddr() override
    { 
        return reinterpret_cast<uint32_t*>(m_buffer); 
    }
    const uint32_t* getAddr() const override
    {
        return reinterpret_cast<const uint32_t*>(m_buffer); 
    }

    size_t getSize() const override
    { 
        return BufferSize;
    }

    uint8_t& operator[](size_t index) override
    {
        return m_buffer[index];
    }
    
    const uint8_t& operator[](size_t index) const override
    {
        return m_buffer[index];
    }

    void clean() const override
    {
        auto cacheAddr = reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(m_buffer));
        SCB_CleanDCache_by_Addr(cacheAddr, BufferSize);
    }

    void invalidate() override
    {
        if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U)
        {
            auto cacheAddr = reinterpret_cast<uint32_t*>(m_buffer);
            SCB_InvalidateDCache_by_Addr(cacheAddr, BufferSize);
        }
    }
};

}   // namespace dma
}   // namespace driver
