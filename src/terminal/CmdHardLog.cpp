#include "CmdHardLog.h"
#include <cstring>
#include "driver/mem/Flash.h"
#include "driver/nvic/Nvic.h"

namespace console {

CmdHardFaultLog cmdFaultLog;

using namespace driver::hardfault;

void DecodeHardFault(const volatile HardFaultLog& log)
{
    Log()  << "\r\n========================================\r\n";
    Log()  << "     CRITICAL FAULT ANALYSIS (H7)       \r\n";
    Log()  << "========================================\r\n";
    Log()  << "Fault Address (PC):  " << log.pc << "\r\n";
    Log()  << "Return Address (LR): " << log.lr << "\r\n";
    Log()  << "SCB->CFSR Register:  " << log.cfsr << "\r\n";
    Log()  << "SCB->HFSR Register:  " << log.hfsr << "\r\n";

    // 1. USAGE FAULT ANALYSIS
    if (log.cfsr & (0xFFFF0000U)) {
        Log()  << "\r\n[Type]: Usage Fault (Execution Logic Error)\r\n";
        if (log.cfsr & SCB_CFSR_DIVBYZERO_Msk)   Log()  << " -> Reason: Division by zero!\r\n";
        if (log.cfsr & SCB_CFSR_UNALIGNED_Msk)   Log()  << " -> Reason: Unaligned memory access!\r\n";
        if (log.cfsr & SCB_CFSR_UNDEFINSTR_Msk)  Log()  << " -> Reason: Undefined instruction executed!\r\n";
        if (log.cfsr & SCB_CFSR_INVSTATE_Msk)    Log()  << " -> Reason: Invalid Thumb Mode state (EPSR T-bit)!\r\n";
        if (log.cfsr & SCB_CFSR_INVPC_Msk)       Log()  << " -> Reason: Invalid EXC_RETURN code in LR!\r\n";
        if (log.cfsr & SCB_CFSR_NOCP_Msk)        Log()  << " -> Reason: Attempted to access disabled FPU!\r\n";
    }

    // 2. BUS FAULT ANALYSIS
    if (log.cfsr & (0x0000FF00U)) {
        Log()  << "\r\n[Type]: Bus Fault (Hardware System Bus Error)\r\n";
        if (log.cfsr & SCB_CFSR_IBUSERR_Msk)     Log()  << " -> Reason: Instruction bus error!\r\n";
        if (log.cfsr & SCB_CFSR_PRECISERR_Msk)   Log()  << " -> Reason: Precise data bus error (read/write).\r\n";
        if (log.cfsr & SCB_CFSR_IMPRECISERR_Msk) Log()  << " -> Reason: Imprecise data bus error.\r\n";
        if (log.cfsr & SCB_CFSR_STKERR_Msk)      Log()  << " -> Reason: Bus error during PUSH (Stack Overflow)!\r\n";
        if (log.cfsr & SCB_CFSR_UNSTKERR_Msk)    Log()  << " -> Reason: Bus error during POP!\r\n";
        
        if (log.cfsr & SCB_CFSR_BFARVALID_Msk) {
            Log()  << " -> FAULTING MEMORY ADDRESS (BFAR): " << log.bfar << "\r\n";
            
            if (log.bfar == 0x00000000)      Log()  << "    [Hint]: Null pointer dereference (nullptr).\r\n";
            else if (log.bfar == 0xFFFFFFFF) Log()  << "    [Hint]: Out-of-bounds address (simulated crash).\r\n";
        }
    }

    // 3. MEMMANAGE FAULT ANALYSIS
    if (log.cfsr & (0x000000FFU)) {
        Log()  << "\r\n[Type]: MemManage Fault (MPU Access Violation)\r\n";
        if (log.cfsr & SCB_CFSR_IACCVIOL_Msk)  Log()  << " -> Reason: Instruction access violation!\r\n";
        if (log.cfsr & SCB_CFSR_DACCVIOL_Msk)  Log()  << " -> Reason: Data access violation!\r\n";
        if (log.cfsr & SCB_CFSR_MSTKERR_Msk)   Log()  << " -> Reason: MPU fault during stacking!\r\n";
        if (log.cfsr & SCB_CFSR_MUNSTKERR_Msk) Log()  << " -> Reason: MPU fault during unstacking!\r\n";
        
        if (log.cfsr & SCB_CFSR_MMARVALID_Msk) {
            Log()  << " -> MPU VIOLATION ADDRESS (MMFAR): " << log.mmar << "\r\n";
        }
    }

    // 4. DATA CONTEXT
    Log()  << "\r\n[Data Context]:\r\n";
    Log()  << " R0 = " << log.r0 << "    R1 = " << log.r1 << "\r\n";
    Log()  << " R2 = " << log.r2 << "    R3 = " << log.r3 << "\r\n";
    Log()  << "========================================\r\n\r\n";
}

void CmdHardFaultLog::exec (uint32_t argc, char** arg)
{
    Log() << Log::endl;

    /* IMPORTANT FOR H7: Reset the data cache for the area where the log is located 
       so processor reads the actual data directly from the physical Flash:  */
    SCB_InvalidateDCache_by_Addr((uint32_t*)FlashLogAddr, sizeof(HardFaultLog));

    volatile HardFaultLog* flashLog = (volatile HardFaultLog*)FlashLogAddr;
    if (flashLog)
    {
        Log() << Log::Base::Hex << "Preamble: " << flashLog->magic << Log::endl;
        Log() << Log::Base::Hex << "R0:   " << flashLog->r0 << Log::endl;
        Log() << Log::Base::Hex << "R1:   " << flashLog->r1 << Log::endl;
        Log() << Log::Base::Hex << "R2:   " << flashLog->r2 << Log::endl;
        Log() << Log::Base::Hex << "R3:   " << flashLog->r3 << Log::endl;
        Log() << Log::Base::Hex << "R12:  " << flashLog->r12 << Log::endl;
        Log() << Log::Base::Hex << "LR:   " << flashLog->lr << Log::endl;
        Log() << Log::Base::Hex << "PC:   " << flashLog->pc << Log::endl;
        Log() << Log::Base::Hex << "PSR:  " << flashLog->psr << Log::endl;
        Log() << Log::Base::Hex << "HFSR: " << flashLog->hfsr << Log::endl;
        Log() << Log::Base::Hex << "CFSR: " << flashLog->cfsr << Log::endl;
        Log() << Log::Base::Hex << "MMAR: " << flashLog->mmar << Log::endl;
        Log() << Log::Base::Hex << "BFAR: " << flashLog->bfar << Log::endl;

        HardFaultLog logSaved;
        memcpy(&logSaved, (const void*)flashLog, sizeof(HardFaultLog));

        DecodeHardFault(logSaved);
    }
    else
    {
        Log() << "Flash reading failed" << Log::endl;
    }

    Log::enable ();
}

}   // namespace console
