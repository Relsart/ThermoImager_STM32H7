#include "Can.h"
#include <string.h>
#include "driver/Dwt.h"
#include "driver/Rcc.h"
#include "utility/Math.h"
#include "Loger.h"

namespace driver {
namespace can {

static const uint32_t messageRamAddr = 0x4000AC00;  // Start address of CAN Message area in RAM
static const uint32_t messageRamSize = 0x2800;      // Size of CAN Message area (in bytes)
bool Bus::firstStart = true;

Bus::Bus(FDCAN_GlobalTypeDef* can) : 
    m_can(can),
    m_canNumber(can == FDCAN1 ? 0 : 1),
    m_ExtFiltersAddr(messageRamAddr + (m_ExtFiltersOffset * sizeof(uint32_t)) + (m_canNumber * m_ExtFiltersMaxCount * m_ExtFilterWordSize * sizeof(uint32_t))),
    m_RxFifoAddr(messageRamAddr + (m_RxFifoOffset * sizeof(uint32_t)) + (m_canNumber * m_RxFifoPacksCount * m_FifoPackWordSize * sizeof(uint32_t))),
    m_TxFifoAddr(messageRamAddr + (m_TxFifoOffset * sizeof(uint32_t)) + (m_canNumber * m_TxFifoPacksCount * m_FifoPackWordSize * sizeof(uint32_t))),
    receiver(can, m_RxFifoAddr),
    m_txBuffer(256)
{
    int a = 0;
    a++;
}

bool Bus::enterInitMode()
{
    SET_BIT (m_can->CCCR, FDCAN_CCCR_INIT);
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(m_initTimeoutMs);
    while (!READ_BIT(m_can->CCCR, FDCAN_CCCR_INIT))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return false;     // Timeout!
    }
    return true;
}

bool Bus::exitInitMode()
{
    CLEAR_BIT(m_can->CCCR, FDCAN_CCCR_INIT);
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(m_initTimeoutMs);
    while (READ_BIT(m_can->CCCR, FDCAN_CCCR_INIT))
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return false;     // Timeout!
    }
    return true;
}

void Bus::init(Config config)
{
    m_config = config;

    // Clock enable:
    SET_BIT(RCC->APB1HENR, RCC_APB1HENR_FDCANEN);

    // Start initialization:
    if (!enterInitMode()) return;
    SET_BIT(m_can->CCCR, FDCAN_CCCR_CCE);   // Enable configuration changing

    // CAN Message area clearing at first initialization:
    if (firstStart)
    {
        memset(reinterpret_cast<void*>(messageRamAddr), 0, messageRamSize);
        firstStart = false;
    }

    // CAN Messages areas in RAM allocation:
    const uint32_t stdFiltersAddr = m_StdFiltersMaxCount * m_StdFilterWordSize * m_canNumber;
    const uint32_t extFiltersAddr = m_ExtFiltersOffset + m_ExtFiltersMaxCount * m_ExtFilterWordSize * m_canNumber;
    const uint32_t rxFifoAddr = m_RxFifoOffset + m_RxFifoPacksCount * m_FifoPackWordSize * m_canNumber;
    const uint32_t txFifoAddr = m_TxFifoOffset + m_TxFifoPacksCount * m_FifoPackWordSize * m_canNumber;
    WRITE_REG(m_can->SIDFC, (stdFiltersAddr << FDCAN_SIDFC_FLSSA_Pos) | (m_StdFiltersMaxCount << FDCAN_SIDFC_LSS_Pos));
    WRITE_REG(m_can->XIDFC, (extFiltersAddr << FDCAN_XIDFC_FLESA_Pos) | (m_ExtFiltersMaxCount << FDCAN_XIDFC_LSE_Pos));
    WRITE_REG(m_can->RXF0C, (rxFifoAddr << FDCAN_RXF0C_F0SA_Pos) | (m_RxFifoPacksCount << FDCAN_RXF0C_F0S_Pos));
    WRITE_REG(m_can->TXBC, (txFifoAddr << FDCAN_TXBC_TBSA_Pos) | (m_TxFifoPacksCount << FDCAN_TXBC_TFQS_Pos));

    // CAN bitrate configuration:
    const float samplePoint = 0.7;  // TODO -> to argument configs
    // Calculate total count of Time Quantums per 1 Nominal Bit Time:
    // -3 at the end is summ of 1 syncronisation quantum and decrements of seg1 and seg2 (in NBTP register values are -1 of using in hardware)
    uint32_t tqInNbt = driver::Rcc::getInstance().getFreq(reinterpret_cast<uint32_t>(m_can)) / (m_config.bitrateKbs * 1000) - 3;
    // Distributing time quantums between Seg1 and Seg2 according to the Sample Point value:
    uint32_t seg2 = tqInNbt * (1 - samplePoint);
    uint32_t seg1 = tqInNbt - seg2;
    CLEAR_REG(m_can->NBTP);
    CLEAR_REG(m_can->DBTP); // CanFD mode has not supported yet... 
    WRITE_REG(m_can->NBTP,(seg2 << FDCAN_NBTP_NTSEG2_Pos) | (seg1 << FDCAN_NBTP_NTSEG1_Pos));
    
    // Interruptions enabling:
    SET_BIT (m_can->IE, FDCAN_IE_RF0NE |    /* Rx FIFO_0 new message interrupt enable */
                        FDCAN_IE_RF0FE |    /* Rx FIFO_0 full interrupt enable */
                        FDCAN_IE_RF0LE |    /* Rx FIFO_0 message lost interrupt enable */
                        FDCAN_IE_TFEE  |    /* Tx FIFO empty interrupt enable */
                        FDCAN_IE_BOE);      /* Bus_Off status */
    SET_BIT (m_can->ILE, FDCAN_ILE_EINT0);  // Enable interrupt Line 0

    // Other configurations:
    CLEAR_REG(m_can->TXEFC);    // Clear the FIFO event configurations
    WRITE_REG(m_can->GFC, FDCAN_GFC_ANFS | FDCAN_GFC_ANFE);     // Reject frames non-matching with any filter
    CLEAR_BIT (m_can->CCCR, FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE); // Disable FD and bitrate switching (Clear by default!)
    uint32_t tmpConfig = m_can->CCCR;
    MODIFY_REG(tmpConfig, FDCAN_CCCR_DAR_Msk, (m_config.oneShotMode ? 1 : 0) << FDCAN_CCCR_DAR_Pos);    // One-shot mode
    MODIFY_REG(tmpConfig, FDCAN_CCCR_MON_Msk, (m_config.listenOnlyMode ? 1 : 0) << FDCAN_CCCR_MON_Pos); // Silent mode TODO: CHECK!!!
    WRITE_REG(m_can->CCCR, tmpConfig);

    // Finishing initialization:
    if (!exitInitMode()) return;
    #ifndef WITH_RTOS
    receiver.m_irqHandler.m_txFinishedSignal.connect(this); // "Transmitting finished" signal 
    #endif
}

void Bus::init(uint32_t bitrate)
{
    Config cfg { .bitrateKbs = bitrate, .oneShotMode = false, .listenOnlyMode = false };
    init(cfg);
}

void Bus::addFilter(Filter& newFilter)
{
    if (!newFilter.m_signal.isConnected())
        Log(lmSystem, Warn) << "CAN Filter ID " << Log::Base::Hex << newFilter.m_id << ", mask " << newFilter.m_mask << " not connected to any slots!";

    // At first check: is there the same filter (the same id and mask) in the list:
    for (auto filter : receiver.m_filtersList)
    {
        if (filter.m_id == newFilter.m_id && filter.m_mask == newFilter.m_mask)
        {
            // Then only add it in the list (for connecting signal):
            receiver.m_filtersList.push_back(newFilter);
            return;
        }
    }

    // There are no such filter values in the list:
    if (m_ExtFiltersCount >= m_ExtFiltersMaxCount)
        return; // Extended filters count overflowed! TODO: Error log here

    if (!enterInitMode()) 
        return;

    volatile auto filterAddr = reinterpret_cast<uint32_t*>(m_ExtFiltersAddr + sizeof(ExtFilterElement) * m_ExtFiltersCount);
    filterAddr[0] = newFilter.m_id | (static_cast<uint32_t>(EfecVal::Fifo_0_Store) << 29);
    filterAddr[1] = newFilter.m_mask | (static_cast<uint32_t>(EftiVal::Classic) << 30);
    m_ExtFiltersCount++;
    exitInitMode();

    receiver.m_filtersList.push_back(newFilter);
}

void Bus::run(Message* msg, uint32_t)
{
    if (msg)
        send(*msg);          // It's the message from external signal - to buffer and send
    else
        txFromBufferToCan(); // It's the signal "Transmitting finished" - check for unsended tx messages
}

void Bus::send(Message &msg)
{
    if (!m_txBuffer.write(msg))
        Log(lmSystem, Error) << "CAN_" << (m_canNumber ? "2" : "1") << ": Tx buffer overloaded";
    txFromBufferToCan();
}

void Bus::txFromBufferToCan()
{
    Message msg;
    const uint32_t extIdMask = 0x1FFFFFFF;

    const uint8_t rmtBitPos = 29;       // Remote frame flag position
    const uint8_t extIdBitPos = 30;     // Extended ID flag position
    const uint8_t dlcBitPos = 16;       // Data Length Code position
    
    while (!READ_BIT(m_can->TXFQS, FDCAN_TXFQS_TFQF) && m_txBuffer.read(msg))
    {
        msg.id &= extIdMask;
        volatile uint8_t tx_index = (m_can->TXFQS & FDCAN_TXFQS_TFQPI_Msk) >> FDCAN_TXFQS_TFQPI_Pos;
        volatile auto fifo = reinterpret_cast<uint32_t*>(m_TxFifoAddr + tx_index * sizeof(FifoElement));

        // Transmitting FIFO element consists of 4 parts (32-bit words):
        // 1 - First part is CAN ID + flags:
        if (msg.extended)
            fifo[0] = msg.id | (msg.remote << rmtBitPos) | (1 << extIdBitPos);
        else
            fifo[0] = msg.id << 18;
        // 2 - Data length code:
        fifo[1] = msg.length << dlcBitPos;
        // 3 - Data: two words of 4 frames in a row
        fifo[2] = (msg.data[3] << 24) | (msg.data[2] << 16) | (msg.data[1] << 8) | msg.data[0];
        fifo[3] = (msg.data[7] << 24) | (msg.data[6] << 16) | (msg.data[5] << 8) | msg.data[4];

        SET_BIT(m_can->TXBAR, 1UL << tx_index);     // Add the transmission request
    }
}

}   // namespace uart
}   // namespace can
