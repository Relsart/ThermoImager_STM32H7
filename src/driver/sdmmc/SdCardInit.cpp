#include "SdCardInit.h"
#include "driver/Rcc.h"
#include "driver/Dwt.h"
#include "Loger.h"

namespace driver {
namespace sdmmc {

Log& operator<<(Log& out, const CardSpeedClass& speedClass)
{
    switch (speedClass)
    {
    case CardSpeedClass::Class0: out << "Class_0"; break;
    case CardSpeedClass::Class2: out << "Class_2"; break;
    case CardSpeedClass::Class4: out << "Class_4"; break;
    case CardSpeedClass::Class6: out << "Class_6"; break;
    case CardSpeedClass::Class10: out << "Class_10"; break;
    }
    return out;
}

Configurator::Configurator(SDMMC_TypeDef* sdmmc, CommandSender& cmdSender) : m_sdmmc(sdmmc), m_cmdSender(cmdSender)
{
}

void Configurator::setBusClockControl(Config& config)
{
    if (config.clockDiv % 2)
        Log(lmSDMMC, Warn) << "Bus clock divider must be an event value. Set to " << --config.clockDiv;

    uint32_t divValue = config.clockDiv / 2;  // Divider Value writing in register is /2 of target:
    const uint32_t MaxClockDivider = 0x3FF;
    uint32_t tmpreg = (divValue > MaxClockDivider) ? MaxClockDivider : divValue;
    tmpreg |= config.clockPowerSave << SDMMC_CLKCR_PWRSAV_Pos;
    tmpreg |= (static_cast<uint8_t>(config.busWide)) << SDMMC_CLKCR_WIDBUS_Pos;
    tmpreg |= (static_cast<uint8_t>(config.clockEdge)) << SDMMC_CLKCR_NEGEDGE_Pos;
    tmpreg |= (static_cast<uint8_t>(config.hwFlowCtrl)) << SDMMC_CLKCR_HWFC_EN_Pos;
    const uint32_t ClkcrClrMsk = 0x3fd3ff;  // CLKCR register clearing mask
    MODIFY_REG(m_sdmmc->CLKCR, ClkcrClrMsk, tmpreg);
}

void Configurator::dataTransferConfig(const DataConfig& config)
{
    const uint32_t MaxDataLenght = 0x1FFFFFF;
    WRITE_REG(m_sdmmc->DTIMER, config.timeOut);
    WRITE_REG(m_sdmmc->DLEN, (config.dataLength > MaxDataLenght ? MaxDataLenght : config.dataLength));

    const uint32_t DctrlClrMsk = 0xFF;    // DCTRL register clearing mask
    uint32_t tmpreg = (static_cast<uint8_t>(config.blockSize)) << SDMMC_DCTRL_DBLOCKSIZE_Pos;
    tmpreg |= (static_cast<uint8_t>(config.direction)) << SDMMC_DCTRL_DTDIR_Pos;
    tmpreg |= (static_cast<uint8_t>(config.mode)) << SDMMC_DCTRL_DTMODE_Pos;
    tmpreg |= (static_cast<uint8_t>(config.dpsm)) << SDMMC_DCTRL_DTEN_Pos;
    MODIFY_REG(m_sdmmc->DCTRL, DctrlClrMsk, tmpreg);
}

bool Configurator::goToIdleState(Result& result)
{
    Command cmd
    {
        .Argument = 0,
        .CmdIndex = Commands::GoIdleState,
        .Response = ResponseSize::NoResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    return m_cmdSender.sendCommand(cmd, ResponseType::None, result);
}

bool Configurator::toApplicationCmd(uint32_t arg, Result& result)
{
    Command cmd
    {
        .Argument = arg,
        .CmdIndex = Commands::ApplicationCmd,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    return m_cmdSender.sendCommand(cmd, ResponseType::R1, result);
}

bool Configurator::readCardVers(CardVersion& version, Result& result)
{
    const uint32_t ArgVersCheck = 0x1AA;
    Command cmd
    {
        .Argument = ArgVersCheck,
        .CmdIndex = Commands::GetInterfaceCond,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R7, result))
    {
        version = CardVersion::V1;     // If it's no response to CMD_8 command- it is v1.0
        return goToIdleState(result);  // Switching to Idle mode again
    }
    else
    {
        version = CardVersion::V2;
        return toApplicationCmd(0, result);
    }
}

bool Configurator::readCapacityType(CapacityType& type, Result& result)
{
    uint32_t response = 0;
    Command cmd
    {
        .Argument = 0x80100000 | 0x40000000 | 0x01000000,
        .CmdIndex = Commands::SD_SendOpCond,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };

    const uint32_t MaxVoltTrial = 0x1000;   // Number of retries of SendOpCond commands sending at powering on (was 0xFFFF)
    uint32_t count = 0;             // Retries counter
    bool powerUpReady = false;      // Chip power is On
    while(!powerUpReady)
    {
        if(count++ == MaxVoltTrial)
        {
            result = Result::PowerOnFail;
            return false;
        }

        if (!toApplicationCmd(0, result))
            return false;

        if (!m_cmdSender.sendCommand(cmd, ResponseType::R3, result))
            return false;

        m_cmdSender.getResponse(ResponseSize::ShortResponse, &response);
        const uint32_t powerUp = 0x80000000;    // Power on is complete (bit 31)
        powerUpReady = ((response & powerUp) != 0);
    }

    const uint32_t highCapacity = 0x40000000;   // Large-capacity SD card (bit 30)
    type = (response & highCapacity) ? CapacityType::HighExtCapacity : CapacityType::StandartCapacity;
    return true;
}

bool Configurator::readCid(uint32_t* cid, Result& result)
{
    if (!cid)
    {
        result = Result::ArgError;
        return false;
    }
    Command cmd
    {
        .Argument = 0,
        .CmdIndex = Commands::GetCid,
        .Response = ResponseSize::LongResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R2, result))
        return false;

    m_cmdSender.getResponse(ResponseSize::LongResponse, cid);
    return true;
}

bool Configurator::readRca(uint32_t* rca, Result& result)
{
    if (!rca)
    {
        result = Result::ArgError;
        return false;
    }

    // By sending the RCA setting command with a null argument, get its current RCA
    Command cmd
    {
        .Argument = 0,
        .CmdIndex = Commands::SetRelativeAddr,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    uint32_t tempRca = 0;
    const uint32_t TimeOut = 1000;  // Command executing timeout (in milliseconds)
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(TimeOut);
    while (tempRca == 0)
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
        {
            result = Result::ProgramTimeout;
            return false;
        }

        if (!m_cmdSender.sendCommand(cmd, ResponseType::R6, result))
            return false;
        if (m_sdmmc->RESPCMD != static_cast<uint32_t>(Commands::SetRelativeAddr))
        {
            result = Result::StatusCmdError;
            return false;
        }

        uint32_t response = 0;
        m_cmdSender.getResponse(ResponseSize::ShortResponse, &response);
        // The lower 16 bytes in resp6 are the status of the card, they need to be checked for errors:
        const uint16_t R6UnknownError = 0x2000;     // Flag #19 General/unknown error
        const uint16_t R6IllegalCmd = 0x4000;       // Flag #22 Command not legal
        const uint16_t R6CrcError = 0x8000;         // Flag #23 CRC of a command failed
        if (static_cast<uint16_t>(response) & (R6UnknownError | R6IllegalCmd | R6CrcError))
        {
            result = Result::CardStatusError;
            return false;
        }
        tempRca = response >> 16;
    }

    *rca = tempRca;
    result = Result::Ok;
    return true;
}

bool Configurator::readCSD(uint32_t rca, CapacityType cardType, Result& result)
{
    Command cmd
    {
        .Argument = rca << 16,
        .CmdIndex = Commands::GetCSD,
        .Response = ResponseSize::LongResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R2, result))
        return false;

    // CSD structs for different versions:
    union 
    {
        SdCsdReg1 csdStruct1;
        SdCsdReg2 csdStruct2;
        uint32_t csd[4]{0};
    };

    m_cmdSender.getResponse(ResponseSize::LongResponse, csd);
    uint8_t scdVersion = csdStruct1.StructVers;

    // Get data from CSD registter: block size, blocks number and erasing permission:
    const uint16_t CmdEraseClass = 0x20;   // Bit 5 of the field Command Classes in CSD register
    if (cardType == CapacityType::StandartCapacity)
    {
        m_deviceSize = (uint32_t)csdStruct1.DevSize2 | ((uint32_t)csdStruct1.DevSize1 << 2);
        uint32_t mult = 1 << (csdStruct1.DevSizeMult + 2);  // 2 ^ (mult + 2)
        m_cardInfo.blocksNumber = (m_deviceSize + 1) * mult;
        m_cardInfo.blockSize = 1 << csdStruct1.MaxReadDataBlockLen;
        m_cardInfo.erasingSupport = (csdStruct1.CardCmdClasses & CmdEraseClass) ? true : false;
    }
    else if (cardType == CapacityType::HighExtCapacity)
    {
        m_deviceSize = (uint32_t)csdStruct2.DevSize2 | ((uint32_t)csdStruct2.DevSize1 << 16);
        m_cardInfo.blocksNumber = (m_deviceSize + 1) * 1024;
        m_cardInfo.blockSize = 512; // Fixed
        m_cardInfo.erasingSupport = (csdStruct2.CardCmdClasses & CmdEraseClass) ? true : false;
    }
    else
    {
        result = Result::CardTypeFail;
        return false;
    }
    uint64_t cardByteSize = static_cast<uint64_t>(m_cardInfo.blocksNumber) * static_cast<uint64_t>(m_cardInfo.blockSize);
    float cardGByteSize = (static_cast<double>(cardByteSize) / static_cast<double>(0x40000000));
    Log(lmSDMMC, Info) << "Get card CSD data: Card capacity: " << cardGByteSize << " Gbytes (" << cardByteSize << " bytes)";
    //Log(lmSDMMC, Info) << "Blocks number: " << blocks << ", one block size: " << blockLen << ", erasing enable: " << (supportErasing ? "Y" : "N");
    return true;
}

bool Configurator::readSCR(SCR& scrStruct, Result& result)
{
    Command cmd
    {
        .Argument = 0,
        .CmdIndex = Commands::GetSCR,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
        return false;

    uint32_t endFlags = SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DBCKEND | SDMMC_STA_DATAEND;
    uint32_t tempSCR[2]{0};
    bool readComplete = false;
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(5000);
    while (!READ_BIT(m_sdmmc->STA, endFlags))
    {
        if (!READ_BIT(m_sdmmc->STA, SDMMC_STA_RXFIFOE) && !readComplete)
        {
            tempSCR[0] = m_sdmmc->FIFO;
            tempSCR[1] = m_sdmmc->FIFO;
            readComplete = true;
        }
        if (driver::DwtTimer::getInstance().checkTimeout(param))
        {
            result = Result::ProgramTimeout;
            return false;
        }
    }

    // Static flags reset:
    if (READ_BIT(m_sdmmc->STA, SDMMC_STA_DTIMEOUT))
    {
        SET_BIT(m_sdmmc->ICR, SDMMC_STA_DTIMEOUT);
        result = Result::SysDataTimeout;
        return false;
    }
    else if (READ_BIT(m_sdmmc->STA, SDMMC_STA_DCRCFAIL))
    {
        SET_BIT (m_sdmmc->ICR, SDMMC_STA_DCRCFAIL);
        result = Result::SysDataCrcFail;
        return false;
    }
    else if (READ_BIT(m_sdmmc->STA, SDMMC_STA_RXOVERR))
    {
        SET_BIT(m_sdmmc->ICR, SDMMC_STA_RXOVERR);
        result = Result::SysFifoFail;
        return false;
    }

    SET_BIT(m_sdmmc->ICR, m_StaticDataFlags);

    // Rearranging the fields to the desired position:
    uint32_t scrbytes = (((tempSCR[0] & 0x000000FF) << 24)  | ((tempSCR[0] & 0x0000FF00) << 8)  | ((tempSCR[0] & 0x00FF0000) >> 8)  | ((tempSCR[0] & 0xFF000000) >> 24));
    // Get hardvare specification:
    uint8_t spec = (scrbytes & 0xF000000) >> 24;
    uint8_t spec3 = (scrbytes & 0x8000) >> 15;
    uint8_t spec4 = (scrbytes & 0x400) >> 10;
    uint8_t specX = (scrbytes & 0x3C0) >> 6;

    if ((spec == 0) && (spec3 == 0) && (spec4 == 0) && (specX == 0))
        scrStruct.spec = PhysicSpecification::Vers1_0;
    else if ((spec == 1) && (spec3 == 0) && (spec4 == 0) && (specX == 0))
        scrStruct.spec = PhysicSpecification::Vers1_1;
    else if ((spec == 2) && (spec3 == 0) && (spec4 == 0) && (specX == 0))
        scrStruct.spec = PhysicSpecification::Vers2;
    else if ((spec == 2) && (spec3 == 1) && (spec4 == 0) && (specX == 0))
        scrStruct.spec = PhysicSpecification::Vers3;
    else if ((spec == 2) && (spec3 == 1) && (spec4 == 1) && (specX == 0))
        scrStruct.spec = PhysicSpecification::Vers4;
    else if ((spec == 2) && (spec3 == 1) && (specX == 1))
        scrStruct.spec = PhysicSpecification::Vers5;
    else if ((spec == 2) && (spec3 == 1) && (specX == 2))
        scrStruct.spec = PhysicSpecification::Vers6;
    else if ((spec == 2) && (spec3 == 1) && (specX == 3))
        scrStruct.spec = PhysicSpecification::Vers7;
    else if ((spec == 2) && (spec3 == 1) && (specX == 4))
        scrStruct.spec = PhysicSpecification::Vers8;
    else if ((spec == 2) && (spec3 == 1) && (specX == 5))
        scrStruct.spec = PhysicSpecification::Vers9;

    scrStruct.support1bitBus = ((scrbytes & 0x10000) != 0) ? true : false;
    scrStruct.support4bitBus = ((scrbytes & 0x40000) != 0) ? true : false;

    switch ((scrbytes & 0x700000) >> 20)
    {
    case 0: scrStruct.security = SdSecurity::None;
        break;
    case 1: scrStruct.security = SdSecurity::NotUsed;
        break;
    case 2: scrStruct.security = SdSecurity::SDSC;
        break;
    case 3: scrStruct.security = SdSecurity::SDHC;
        break;
    case 4: scrStruct.security = SdSecurity::SDXC;
        break;
    }

    scrStruct.supportCMD20 = ((scrbytes & 0x1) != 0) ? true : false;
    scrStruct.supportCMD23 = ((scrbytes & 0x2) != 0) ? true : false;
    scrStruct.supportCMD48_49 = ((scrbytes & 0x4) != 0) ? true : false;
    scrStruct.supportCMD58_59 = ((scrbytes & 0x8) != 0) ? true : false;
    scrStruct.supportACMD53_54 = ((scrbytes & 0x10) != 0) ? true : false;

    return true;
}

bool Configurator::cardSelect(uint32_t rca, Result& result)
{
    Command cmd
    {
        .Argument = rca << 16,
        .CmdIndex = Commands::SelectCard,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    if(!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
        return false;

    // Checking card for blocking:
    uint32_t response = 0;
    m_cmdSender.getResponse(ResponseSize::ShortResponse, &response);
    const uint8_t blockBitPos = 25;
    if (response & (1 << blockBitPos))
    {
        result = Result::StatusCardLocked;
        return false;
    }
    return true; 
}

bool Configurator::setBlockSize(BlockSize blockSize, Result& result)
{
    Command cmd
    {
        .Argument = (1 << static_cast<uint8_t>(blockSize)),
        .CmdIndex = Commands::SetBlockLen,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    return m_cmdSender.sendCommand(cmd, ResponseType::R1, result);
}

bool Configurator::readCardExtStatus(Result& result)
{
    // Set block size as 64:
    if (!setBlockSize(BlockSize::Size_64B, result))
    {
        SET_BIT(m_sdmmc->ICR, m_StaticFlags);
        return false;
    }

    // Switching to application commands:
    if (!toApplicationCmd(m_rca << 16, result))
        return false;

    // Data Path State Machine (DPSM) set to 64-byte block:
    DataConfig dataConfig
    {
        .dataLength = 64,
        .blockSize = BlockSize::Size_64B,
        .direction = TransferDir::ToHost,
        .mode = TransferMode::BlockMode,
        .dpsm =  true
    };
    dataTransferConfig(dataConfig);

    // Request the 512-bits status (extended ACMD_13 command):
    Command cmd
    {
        .Argument = 0,
        .CmdIndex = Commands::GetStatus,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    if (!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
        return false;

    uint32_t endFlags = SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DATAEND;
    uint32_t sd_status[16]{0};
    uint8_t index = 0;
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(5000);
    while (!READ_BIT(m_sdmmc->STA, endFlags))
    {
        // Receive FIFO Half Full: there are 8 words in the FIFO - get the data
        if (READ_BIT(m_sdmmc->STA, SDMMC_STA_RXFIFOHF))
        {
            for(int i = 0; i < 8; i++)
                sd_status[index++] = m_sdmmc->FIFO;
        }

        if (driver::DwtTimer::getInstance().checkTimeout(param))
        {
            result = Result::ProgramTimeout;
            return false;
        }
    }

    // Check static flags:
    if (READ_BIT(m_sdmmc->STA, SDMMC_STA_DTIMEOUT))
    {
        result = Result::SysDataTimeout;
        return false;
    }
    else if (READ_BIT(m_sdmmc->STA, SDMMC_STA_DCRCFAIL))
    {
        result = Result::SysDataCrcFail;
        return false;
    }
    else if (READ_BIT(m_sdmmc->STA, SDMMC_STA_RXOVERR))
    {
        result = Result::SysFifoFail;
        return false;
    }

    // Read the rest of data (if there is):
    param = driver::DwtTimer::getInstance().timeoutInit(5000);
    while (READ_BIT(m_sdmmc->STA, SDMMC_STA_DPSMACT))
    {
        sd_status[index++] = m_sdmmc->FIFO;
        if (driver::DwtTimer::getInstance().checkTimeout(param))
        {
            result = Result::ProgramTimeout;
            return false;
        }
    }

    // Clear static data flags:
    SET_BIT(m_sdmmc->ICR, m_StaticDataFlags);

    // Convert received data to the struct:
    switch ((sd_status[0] & 0xC0) >> 6)
    {
    case 0b00: m_cardState.busWidth = BusWide::W1_bit;  break;
    case 0b10: m_cardState.busWidth = BusWide::W4_bits; break;
    }

    m_cardState.securedMode = ((sd_status[0] & 0x20) >> 5) ? true : false;

    switch (((sd_status[0] & 0x00FF0000) >> 8) | ((sd_status[0] & 0xFF000000) >> 24))
    {
    case 0: m_cardState.cardType = CardType::StandartRW; break;
    case 1: m_cardState.cardType = CardType::ReadOnly; break;
    case 2: m_cardState.cardType = CardType::OTP; break;
    }

    m_cardState.protectedAreaSize = (((sd_status[1] & 0xFF) << 24)    | ((sd_status[1] & 0xFF00) << 8) |
                                  ((sd_status[1] & 0xFF0000) >> 8) | ((sd_status[1] & 0xFF000000) >> 24));

    switch (sd_status[2] & 0xFF)
    {
    case 0: m_cardState.speedClass = CardSpeedClass::Class0;  break;
    case 1: m_cardState.speedClass = CardSpeedClass::Class2;  break;
    case 2: m_cardState.speedClass = CardSpeedClass::Class4;  break;
    case 3: m_cardState.speedClass = CardSpeedClass::Class6;  break;
    case 4: m_cardState.speedClass = CardSpeedClass::Class10; break;
    }

    Log(lmSDMMC, Info) << "Get card Speed Class " << m_cardState.speedClass;

    m_cardState.perfomanceMove = (sd_status[2] & 0xFF00) >> 8;

    uint8_t temp = (sd_status[2] & 0xF00000) >> 20;
    if (temp < 16)
        m_cardState.aus = static_cast<AllocateUnitSize>(temp);

    m_cardState.eraseSize = ((sd_status[2] & 0xFF000000) >> 16) | (sd_status[3] & 0xFF);
    m_cardState.eraseTimeout = (sd_status[3] & 0xFC00) >> 10;
    m_cardState.eraseOffset = (sd_status[3] & 0x0300) >> 8;

    switch ((sd_status[3] & 0x00F0) >> 4)
    {
    case 0: m_cardState.uhsSpeedGrade = UHSMode::Less10Mbs; break;
    case 1: m_cardState.uhsSpeedGrade = UHSMode::More10Mbs; break;
    case 3: m_cardState.uhsSpeedGrade = UHSMode::More30Mbs; break;
    }

    temp = sd_status[3] & 0x000F;
    if ((temp > 6) && (temp < 16))
        m_cardState.uhsAus = static_cast<AllocateUnitSize>(temp);

    switch ((sd_status[4] & 0xFF000000) >> 24)
    {
    case 0:  m_cardState.videoSpeed = CardVideoClass::NotSupported;  break;
    case 6:  m_cardState.videoSpeed = CardVideoClass::Class6;  break;
    case 10: m_cardState.videoSpeed = CardVideoClass::Class10; break;
    case 30: m_cardState.videoSpeed = CardVideoClass::Class30; break;
    case 60: m_cardState.videoSpeed = CardVideoClass::Class60; break;
    case 90: m_cardState.videoSpeed = CardVideoClass::Class90; break;    
    }

    // NOTE:
    // Unnecessary command to setting block size as 512 was remover from here.
    // Immediately after it (in getSdStatus) the block size setting as 8

    // Get cards speed mode:
    if ((m_cardInfo.capacityType == CapacityType::HighExtCapacity) && (m_cardState.uhsSpeedGrade != UHSMode::Less10Mbs) && (m_cardState.uhsAus != AllocateUnitSize::None))
        m_cardInfo.cardSpeed = CardSpeed::UltraHigh;
    else if (m_cardInfo.capacityType == CapacityType::HighExtCapacity)
        m_cardInfo.cardSpeed = CardSpeed::High;
    else
        m_cardInfo.cardSpeed = CardSpeed::Normal;
    return true;
}

bool Configurator::setBusWidth(BusWide busWide, Result& result)
{
    if ((busWide != BusWide::W1_bit) && (busWide != BusWide::W4_bits))
    {
        result = Result::ArgError;
        return false;   // For SD cards available only 1 and 4-wire connections
    }

    // Checking card for blocking (in it necessary here?):
    uint32_t response = 0;
    m_cmdSender.getResponse(ResponseSize::ShortResponse, &response);
    const uint8_t blockBitPos = 25;
    if (response & (1 << blockBitPos))
    {
        result = Result::StatusCardLocked;
        return false;
    }

    // Set block size as 8 bytes
    if (!setBlockSize(BlockSize::Size_8B, result))
    {
        SET_BIT(m_sdmmc->ICR, m_StaticFlags);
        return false;
    }

    // Switching to application commands:
    if (!toApplicationCmd(m_rca << 16, result))
        return false;

    // Data Path State Machine (DPSM) configuration to 8-byte block:
    DataConfig dataConfig
    {
        .dataLength = 8,
        .blockSize = BlockSize::Size_8B,
        .direction = TransferDir::ToHost,
        .mode = TransferMode::BlockMode,
        .dpsm =  true
    };
    dataTransferConfig(dataConfig);

    // Get SCR register:
    SCR scrStruct;
    if (!readSCR(scrStruct, result))
        return false;;

    // Checking for target bus width supporting:
    if (((busWide == BusWide::W4_bits) && (!scrStruct.support4bitBus)) || ((busWide == BusWide::W1_bit) && (!scrStruct.support1bitBus)))
    {
        result = Result::OptionNotSupported;
        return false;
    }

    // Switching to application commands:
    if (!toApplicationCmd(m_rca << 16, result))
        return false;

    // Enable 4-wire bus:
    Command cmd
    {
        .Argument = (busWide == BusWide::W4_bits) ? 2 : 0,
        .CmdIndex = Commands::SetBusWidth,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    return m_cmdSender.sendCommand(cmd, ResponseType::R1, result);
}

SdCardState Configurator::readCardState(uint32_t rca)
{
    Result result;
    Command cmd
    {
        .Argument = rca << 16,
        .CmdIndex = Commands::GetStatus,
        .Response = ResponseSize::ShortResponse,
        .WaitForInterrupt = false,
        .CPSM = true
    };
    if(!m_cmdSender.sendCommand(cmd, ResponseType::R1, result))
        return SdCardState::Error;

    uint32_t response = 0;
    m_cmdSender.getResponse(ResponseSize::ShortResponse, &response);
    uint8_t stateValue = (response >> 9) & 0x0F;
    return (stateValue >= static_cast<uint8_t>(SdCardState::Error)) ? SdCardState::Error : static_cast<SdCardState>(stateValue);
}

Result Configurator::init(const Config& config)
{
    Result result;
    m_cardInfo.initialised = false;

    // Periphery Clock enable:
    if (m_sdmmc == SDMMC1)
        SET_BIT(RCC->AHB3ENR, RCC_AHB3ENR_SDMMC1EN);
    else if (m_sdmmc == SDMMC2)
        SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_SDMMC2EN);
    else
        return Result::ArgError;

    /* * * * Startup Bus and Clock confuguration * * * */
    uint32_t sdmmcClk = Rcc::getInstance().getFreq (reinterpret_cast<uint32_t>(m_sdmmc));
    const uint32_t MaxSdmmcClock = 100000000;
    if ((sdmmcClk == 0) || (sdmmcClk > MaxSdmmcClock))
        return Result::ClockError;

    const uint32_t SdInitFreq = 400000; // SDMMC frequency at the initialising must not be more than 400 kHz
    Config defaultConf                  // Default configuration (for SD card)
    {
        .clockEdge = ClockEdge::Rising,
        .busWide = BusWide::W1_bit,
        .clockDiv = sdmmcClk / SdInitFreq,
        .clockPowerSave = false,
        .hwFlowCtrl = false
    };
    setBusClockControl(defaultConf);
    sdmmcClk /= defaultConf.clockDiv;
    Log(lmSDMMC, Info) << "Startup at default frequency " << sdmmcClk << " Hz";

    /* * * * Powering up the module * * * */
    SET_BIT(m_sdmmc->POWER, SDMMC_POWER_PWRCTRL);
    driver::DwtTimer::getInstance().delayMs(1 + (74 * 1000 / sdmmcClk));  // Delay for 74 clock cycles
    if (!goToIdleState(result))
        return result;

    /* * * * Get the card type (V1.0/V2.0) * * * */
    if (!readCardVers(m_cardInfo.cardVers, result))
        return result;
    Log(lmSDMMC, Info) << "Get version of card: " << (m_cardInfo.cardVers == CardVersion::V2 ? "V2" : "V1");

    /* * * * Get Card Capacity Type (Standard or Extended) * * * */
    if (!readCapacityType(m_cardInfo.capacityType, result))
        return result;
    Log(lmSDMMC, Info) << "Get card Capacity Type: " << (m_cardInfo.capacityType == CapacityType::StandartCapacity ? "STD" : "EXT");

    /* * * * Get card CID identificator * * * */
    uint32_t cid[4]{0};
    if (!readCid(cid, result))
        return result;
    Log(lmSDMMC, Info) << "Get card CID: " << Log::Base::Hex << cid[0] << " " << cid[1] << " " << cid[2] << " " << cid[3];

    /* * * * Get Relative Card Address * * * */
    if (!readRca(&m_rca, result))
        return result;
    Log(lmSDMMC, Info) << "Get card RSA: " << Log::Base::Hex << m_rca;

    /* * * * Get data from Card Specific Data register * * * */
    if (!readCSD(m_rca, m_cardInfo.capacityType, result))
        return result;

    /* * * * Select card by its RCA address * * * */
    if (!cardSelect(m_rca, result))
        return result;

    // NOTE:
    // Unnecessary command to setting block size as 512 was remover from here.
    // Immediately after it (in getSdStatus) the block size setting as 64

    /* * * * Request and read the 512-bits cards status * * * */
    if (!readCardExtStatus(result))
        return result;

    /* * * * Set bus width * * * */
    if (!setBusWidth(config.busWide, result))
    {
        SET_BIT(m_sdmmc->ICR, m_StaticFlags);
        return result;
    }

    /* * * * Configure bus speed * * * */
    Config userConfig = config;
    if (m_cardInfo.cardSpeed == CardSpeed::Normal || m_cardInfo.cardSpeed == CardSpeed::High)
    {
        sdmmcClk = Rcc::getInstance ().getFreq(reinterpret_cast<uint32_t>(m_sdmmc));
        const uint32_t SdNormalFreq = 25000000; // Normal frequency: up to 25 MHz
        const uint32_t SdHighFreq = 50000000;   // High frequency: up to 50 MHz
        uint32_t setFreq = sdmmcClk / userConfig.clockDiv;
        uint32_t maxFreq = (m_cardInfo.cardSpeed == CardSpeed::High) ? SdHighFreq : SdNormalFreq;
        if (setFreq > maxFreq)
        {
            userConfig.clockDiv = sdmmcClk / maxFreq;
            Log(lmSDMMC, Warn) << "Frequency is too high: " << setFreq << " Hz, changed to " << (sdmmcClk / userConfig.clockDiv);
        }
        else
        {
            Log(lmSDMMC, Info) << "Set bus frequency " <<  (sdmmcClk / userConfig.clockDiv) << " Hz";
        }
    }
    setBusClockControl(userConfig);

    /* * * * Configure block size as 512 bytes * * * */
    if (!setBlockSize(BlockSize::Size_512B, result))
    {
        SET_BIT(m_sdmmc->ICR, m_StaticFlags);
        return result;
    }

    /* * * * Wait the card to be ready * * * */
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(5000);
    while (readCardState(m_rca) != SdCardState::Transfer)
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param))
            return Result::ProgramTimeout;
    }
    m_cardInfo.initialised = true;
    return Result::Ok;
}

const CardInfo& Configurator::getCardInfo()
{
    return m_cardInfo;
}

}   // namespace sdmmc
}   // namespace driver
