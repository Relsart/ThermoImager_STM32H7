#include "Nvic.h"
#include "driver/mem/Flash.h"
#include "driver/sdmmc/Sdmmc.h"
#include "driver/nvic/NvicManager.h"
#ifdef WITH_RTOS
#include <FreeRTOS.h>
#include <task.h>
#endif

//#define FAULT_LOG_TO_FLASH // Option: saving logs to the Flash memory in Hard fault handler

/**
 * @brief SysTick interruptions handlers
 */
volatile uint64_t msTicks = 0;  // Global System Ticks counter
extern "C" void SysTick_Handler(void)  
{
    #ifdef WITH_RTOS
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();  // It shouldn't be invoked if Task Scheduler doesn't start 
    #endif
    msTicks++;                                                
}

/**
 * @brief UART interruptions handlers
 */
extern "C" void USART1_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getUartNvic(USART1)->activate();
}
extern "C" void USART2_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getUartNvic(USART2)->activate();
}
extern "C" void USART3_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getUartNvic(USART3)->activate();
}
extern "C" void UART4_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getUartNvic(UART4)->activate();
}
extern "C" void UART5_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getUartNvic(UART5)->activate();
}
extern "C" void USART6_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getUartNvic(USART6)->activate();
}
extern "C" void UART7_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getUartNvic(UART7)->activate();
}
extern "C" void UART8_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getUartNvic(UART8)->activate();
}

/**
 * @brief SPI interruptions handlers
 */
extern "C" void SPI1_IRQHandler(void)
{
    driver::NvicManager::getInstance().getSpiNvic(SPI1)->activate();
}
extern "C" void SPI2_IRQHandler(void)
{
    driver::NvicManager::getInstance().getSpiNvic(SPI2)->activate();
}
extern "C" void SPI3_IRQHandler(void)
{
    driver::NvicManager::getInstance().getSpiNvic(SPI3)->activate();
}
extern "C" void SPI4_IRQHandler(void)
{
    driver::NvicManager::getInstance().getSpiNvic(SPI4)->activate();
}
extern "C" void SPI5_IRQHandler(void)
{
    driver::NvicManager::getInstance().getSpiNvic(SPI5)->activate();
}
extern "C" void SPI6_IRQHandler(void)
{
    driver::NvicManager::getInstance().getSpiNvic(SPI6)->activate();
}

/**
 * @brief GPIO EXTI interruptions handlers
 */
extern "C" void EXTI0_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getGpioNvic(0);
}
extern "C" void EXTI1_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getGpioNvic(1)->activate();
}
extern "C" void EXTI2_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getGpioNvic(2)->activate();
}
extern "C" void EXTI3_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getGpioNvic(3)->activate();
}
extern "C" void EXTI4_IRQHandler(void)  
{
    driver::NvicManager::getInstance().getGpioNvic(4)->activate();
}
extern "C" void EXTI9_5_IRQHandler(void) 
{
    driver::NvicManager::getInstance().getGpioNvic(5)->activate();    // Common for pins 5..9
}
extern "C" void EXTI15_10_IRQHandler(void)
{
    driver::NvicManager::getInstance().getGpioNvic(10)->activate();   // Common for pins 10..15
}

/**
 * @brief DMA interruptions handlers
 */
extern "C" void DMA1_Stream0_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA1_Stream0)->activate();
}
extern "C" void DMA1_Stream1_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA1_Stream1)->activate();
}
extern "C" void DMA1_Stream2_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA1_Stream2)->activate();
}
extern "C" void DMA1_Stream3_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA1_Stream3)->activate();
}
extern "C" void DMA1_Stream4_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA1_Stream4)->activate();
}
extern "C" void DMA1_Stream5_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA1_Stream5)->activate();
}
extern "C" void DMA1_Stream6_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA1_Stream6)->activate();
}
extern "C" void DMA1_Stream7_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA1_Stream7)->activate();
}
extern "C" void DMA2_Stream0_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA2_Stream0)->activate();
}
extern "C" void DMA2_Stream1_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA2_Stream1)->activate();
}
extern "C" void DMA2_Stream2_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA2_Stream2)->activate();
}
extern "C" void DMA2_Stream3_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA2_Stream3)->activate();
}
extern "C" void DMA2_Stream4_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA2_Stream4)->activate();
}
extern "C" void DMA2_Stream5_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA2_Stream5)->activate();
}
extern "C" void DMA2_Stream6_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA2_Stream6)->activate();
}
extern "C" void DMA2_Stream7_IRQHandler(void)
{
    driver::NvicManager::getInstance().getDmaNvic(DMA2_Stream7)->activate();
}

/**
 * @brief CAN interruptions handlers
 * @details Now only for interruptions lines 0 (yet)
 */
extern "C" void FDCAN1_IT0_IRQHandler(void)
{
    driver::NvicManager::getInstance().getCanNvic(FDCAN1)->activate();
}
extern "C" void FDCAN2_IT0_IRQHandler(void)
{
    driver::NvicManager::getInstance().getCanNvic(FDCAN2)->activate();
}

/**
 * @brief I2C events interruptions handlers
 */
extern "C" void I2C1_EV_IRQHandler(void)
{
    driver::NvicManager::getInstance().getI2CNvic(I2C1)->activate();
}

extern "C" void I2C2_EV_IRQHandler(void)
{
    driver::NvicManager::getInstance().getI2CNvic(I2C2)->activate();
}

extern "C" void I2C3_EV_IRQHandler(void)
{
    driver::NvicManager::getInstance().getI2CNvic(I2C3)->activate();
}

extern "C" void I2C4_EV_IRQHandler(void)
{
    driver::NvicManager::getInstance().getI2CNvic(I2C4)->activate();
}

/**
 * @brief SDMMC interruptions handlers
 */
extern "C" void SDMMC1_IRQHandler(void)
{
    driver::NvicManager::getInstance().getSdmmcNvic(SDMMC1)->activate();
}
extern "C" void SDMMC2_IRQHandler(void)
{
    driver::NvicManager::getInstance().getSdmmcNvic(SDMMC2)->activate();
}

/**
 * @brief Hard Fault interruption handler (callback)
 */
extern "C" __attribute__((optimize("O0")))
void hardFaultHandler(uint32_t* stack_address, uint32_t lr_reg)
{
    uint32_t pcIndex = 6;           // Default frame Program Counter index (without FPU)
    if ((lr_reg & (1 << 4)) == 0)   // 4th bit in LR reg is type of frame (with/without the FPU)
        pcIndex = 7;                // Extended frame Program Counter index (offset by FPU)

    // Get saved registers from the stack.
    volatile uint32_t r0  = stack_address[0];
    volatile uint32_t r1  = stack_address[1];
    volatile uint32_t r2  = stack_address[2];
    volatile uint32_t r3  = stack_address[3];
    volatile uint32_t r12 = stack_address[4];
    volatile uint32_t lr  = stack_address[5];   // Link Register (where did the dropped function come from?)
    /* ACHTUNG: indexes [7] and [8] are correct only for Cortex-M4F/M7 (with FPU, Floating-Point Unit) */
    /* For others MCUs last two indexes are [6] and [7] */
    volatile uint32_t pc  = stack_address[pcIndex];   // Program Counter (address of the instruction that caused the HardFault!)
    volatile uint32_t psr = stack_address[pcIndex+1]; // Program Status Register
    // Read the system registers that contain the causes of the failure (available on Cortex?M3, M4, M7, H7...):
    #if defined(__CORTEX_M) && (__CORTEX_M >= 3U)
    volatile uint32_t hfsr = SCB->HFSR;     // HardFault Status Register
    volatile uint32_t cfsr = SCB->CFSR;     // Configurable Fault Status Register
    volatile uint32_t mmar = SCB->MMFAR;    // MemManage Fault Address Register (if active)
    volatile uint32_t bfar = SCB->BFAR;     // BusFault Address Register (if active)
    #endif

    #ifdef FAULT_LOG_TO_FLASH
    /* Write Hard Fault log to the FLASH memory */
    __disable_irq();
    volatile driver::hardfault::HardFaultLog log;
    log.magic = 0xDEADBEEF; // Preamble magic value
    log.r0    = r0;
    log.r1    = r1;
    log.r2    = r2;
    log.r3    = r3;
    log.r12   = r12;
    log.lr    = lr;
    log.pc    = pc;
    log.psr   = psr;
    #if defined(__CORTEX_M) && (__CORTEX_M >= 3U)
    log.hfsr  = hfsr;
    log.cfsr  = cfsr;
    log.mmar  = mmar;
    log.bfar  = bfar;
    #endif
    flashUnlockB2();   
    flashEraseSectorB2(driver::hardfault::FlashLogSector);
    flashWrite(driver::hardfault::FlashLogAddr, (uint32_t*)&log, 2);
    flashLockB2();
    #endif

    // --- DEBUG POINT ---
    // Here can be breakpoint for debugger

    // Here can be force reboot:
    // NVIC_SystemReset();

    while (1); // Stack here if MCU has not been rebooted
}

/**
 * @brief Hard Fault interruption handler
 * @details An assembler jump that doesn’t corrupt the stack before the analyzer is called
 */
extern "C" void HardFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4 \n"            // Check bit 2 in the Register Link (LR).
        "ite eq \n"                // If the bit is 0, then
        "mrseq r0, msp \n"         // .. error in the interrupt- take the Main Stack Pointer.
        "mrsne r0, psp \n"         // .. otherwise, there’s an error in the thread- take the Process Stack Pointer.
        "mov r1, lr \n"            // Give LR register value as a second argument (EXC_RETURN, exception return code)
        "b hardFaultHandler \n"    // Invoke the C-function handler (the stack address is already in R0)
    );
}

namespace driver {

void Nvic::init(IRQn_Type _irq, uint8_t priority, SlotInterface<uint8_t>& slot)
{
    if (priority > maxPriorityValue)
        priority = maxPriorityValue;

    m_irq = _irq;
    m_initialised = true;
    if (!IrqSignal.isConnectedTo(&slot))
        IrqSignal.connect(&slot);   // Against re-connection to the same slot

    __NVIC_SetPriority(m_irq, priority);
    __NVIC_EnableIRQ(m_irq);
}

void Nvic::setPriority(uint8_t priority)
{
    if (m_initialised)
    {
        if (priority > maxPriorityValue)
            priority = maxPriorityValue;
        __NVIC_DisableIRQ(m_irq);
        __NVIC_SetPriority(m_irq, priority);
        __NVIC_EnableIRQ(m_irq);
    }
}

void Nvic::activate()
{
    IrqSignal.activ(1);
}

void Nvic::enable()
{
    if (m_initialised)
        __NVIC_EnableIRQ(m_irq);
}

void Nvic::disable()
{
    if (m_initialised)
        __NVIC_DisableIRQ(m_irq);
}

}   // namespace driver
