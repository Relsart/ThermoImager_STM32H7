#pragma once
#include <stdint.h>

namespace driver {
namespace sdmmc {

enum class ClockEdge : uint8_t
{
    Rising = 0,
    Failing = 1
};

enum class BusWide : uint8_t
{
    W1_bit = 0,   // 1-wire bus
    W4_bits = 1,  // 4-wire bus
    W8_bits = 2   // 8-wire bus
};

/**
 * @brief SDMMC main configurations
 */
struct Config
{
    static const uint32_t MaxClockDivider = 0x3FF;
    ClockEdge clockEdge = ClockEdge::Rising;    // Default for SD cards
    BusWide busWide = BusWide::W4_bits;         // Default for SD cards
    uint16_t clockDiv;                          // Clock Divider (must be an event number 0, 2, 4, .. 2046) 
    bool clockPowerSave;
    bool hwFlowCtrl = true;    // Hardware flow control. Recommended to set, else it may be FIFO Overrun problems at high (25MHz) frequences
    uint8_t* idmaBuffer;
};

enum class TransferMode : uint8_t
{
    BlockMode = 0,  // Режим передачи блоками по счетчику блоков 
    StreamMode = 2  // Потоковый режим
};

enum class TransferDir : uint8_t
{
    ToCard = 0,
    ToHost = 1
};

enum class BlockSize : uint8_t
{
    Size_1B = 0,
    Size_2B = 1,
    Size_4B = 2,
    Size_8B = 3,
    Size_16B = 4,
    Size_32B = 5,
    Size_64B = 6,
    Size_128B = 7,
    Size_256B = 8,
    Size_512B = 9,
    Size_1024B = 10,
    Size_2048B = 11,
    Size_4096B = 12,
    Size_8192B = 13,
    Size_16384B = 14
};

/**
 * @brief Configs for Data Transactions
 */
struct DataConfig
{
    static const uint32_t DataTimeOut = 0xFFFFFFFF; 
    uint32_t timeOut = DataTimeOut;     // Data transferring timeout (in clock cycles)
    uint32_t dataLength;                // Length of transferring data in bytes
    BlockSize blockSize;                // One ddata block size in bytes
    TransferDir direction;              // Data transferring direction
    TransferMode mode;                  // Transferring mode: block counting or streaming
    bool dpsm;                          // Start transaction without CPSM command
};

/**
 * @brief SD ard state
 */
enum class SdCardState : uint8_t
{
    Idle = 0,
    Ready = 1, 
    Identification = 2,
    Standby = 3,
    Transfer = 4,
    Sending = 5,
    Receiving = 6,
    Programming = 7,
    Disconnected = 8,
    Error = 9
};

/**
 * @brief Card type
 */
enum class CardType : uint8_t
{
    None,           // Undefined
    StandartRW,     // Simple card for reading/writing
    ReadOnly,       // Only reading card
    OTP             // One time programming card
};

/**
 * @brief Card speed classes
 */
enum class CardSpeedClass : uint8_t
{
    None,
    Class0,
    Class2,
    Class4, 
    Class6,
    Class10
};

enum class CardVideoClass : uint8_t
{
    NotSupported,
    Class6,
    Class10, 
    Class30,
    Class60,
    Class90,
};

/**
 * @brief Card speed
 */
enum class CardSpeed : uint8_t
{
    None,
    Normal,
    High,
    UltraHigh

};
/**
 * @brief SD card versions
 */
enum class CardVersion : uint8_t
{
    None = 0,
    V1 = 1,
    V2 = 2
};

/**
 * @brief Result of SD operations
 */
enum class Result : uint8_t
{
    Ok = 0,             // Successfully
    ArgError,           // Arguments error: null pointer etc
    ClockError,         // SDMMC module clock error
    ProgramTimeout,     // Software timeout (from driver)
    SysCmdTimeout,      // Hardware command timeout
    SysDataTimeout,     // Hardware data timeout
    SysCmdCrcFail,      // Data CRC failed
    SysDataCrcFail,     // Command CRC failed
    SysFifoFail,        // FIFO error: overloaded at receiving or empty at transmitting
    CardStatusError,    // State card error (response 1, 6)
    StatusCardLocked,   // Card is blocked (bit 25)
    StatusOutOfRange,   // State error (bit 31): command argument out of limit
    StatusAddrError,    // State error (bit 30): address offset error (doesn't match to the blocks length)
    StatusBlockLenErr,  // State error (bit 29): block size not supported or package size doesn't match to the blocks length
    StatusEraseError,   // State error (биты 27, 28): erasing error: incorrect sequence of operations or block ranges
    StatusWriteProtect, // State error (bit 26): attempt to write to a write-protected area
    StatusUnlockFail,   // State error (bit 24): failure to unlock the card
    StatusCrcError,     // State error (bit 23): CRC error of the previous command
    StatusCmdError,     // State error (bit 22): wrong command
    StatusEccError,     // State error (bit 21): internal ECC card error
    StatusCtrlError,    // State error (bit 20): failure of the internal card controller
    StatusOtherError,   // State error (bit 19): major or unknown error
    StatusCsdError,     // State error (bit 16): CSD register error
    StatusEraseSkip,    // State error (bit 15): erasing blocks or protected areas failure
    StatusIdentError,   // State error (bit 3): error in the card identification sequence
    PowerOnFail,        // Card power-on error (not ready)
    CardTypeFail,       // Cards type undefined
    OptionNotSupported, // This option is not supported
    IdmaBusy,           // Internal DMA stream is busy
    CardNotInitialised, // Card not initialised
    OtherAnyError       // Other errors
};

/**
 * @brief Cell size
 */
enum class AllocateUnitSize : uint8_t
{
    None = 0,
    Def16Kb = 1,
    Def32Kb = 2,
    Def64Kb = 3,
    Def128Kb = 4,
    Def256Kb = 5,
    Def512Kb = 6,
    Def1Mb = 7,
    Def2Mb = 8,
    Def4Mb = 9,
    Def8Mb = 10,
    Def12Mb = 11,
    Def16Mb = 12,
    Def24Mb = 13,
    Def32Mb = 14,
    Def64Mb = 15
};

/**
 * @brief Ultra High Speed modes
 */
enum class UHSMode : uint8_t
{
    None,
    Less10Mbs,
    More10Mbs,
    More30Mbs,
};

/**
 * @brief Cards capacity types
 */
enum class CapacityType : uint8_t
{
    None,               // Not initialised
    StandartCapacity,   // To 2GB
    HighExtCapacity     // High (from 2GB to 32GB) or Extended (from 32GB to 2TB)
};

/**
 * @brief Physical Layer Specification Version Number.
 */
enum class PhysicSpecification : uint8_t
{
    None,
    Vers1_0,    // Version 1.0 and 1.01
    Vers1_1,    // Version 1.10
    Vers2,      // Version 2.00
    Vers3,      // Version 3.0X
    Vers4,      // Version 4.XX
    Vers5,      // Version 5.XX
    Vers6,      // Version 6.XX
    Vers7,      // Version 7.XX
    Vers8,      // Version 8.XX
    Vers9,      // Version 9.XX
};

/**
 * @brief SD card protection version
 */
enum class SdSecurity : uint8_t
{
    None,       // No protection
    NotUsed,    // Protection not used
    SDSC,       // Standard capacity card (protection V1.01)
    SDHC,       // High capacity card (protection V2.00)
    SDXC,       // Extended capacity card (protection V3.ХХ)
};

/**
 * @brief Info about connected card
 * @details From configurator to main SDMMC class
 */
struct CardInfo
{
    CapacityType capacityType = CapacityType::None; // SD cards capacity type
    CardVersion cardVers = CardVersion::None;       // SD card Version (v1 or v2)
    CardSpeed cardSpeed = CardSpeed::None;          // Card speed (Normal, High, Ultra-high)
    uint16_t blockSize = 0;         // Size of one block in bytes
    uint32_t blocksNumber = 0;      // Total number of blocks
    bool erasingSupport = false;    // Card erasing support
    bool initialised = false;       // Card was successfully initialised
};

/**
 * @brief SD card status structure
 */
struct SdState
{
    BusWide busWidth = BusWide::W1_bit;                     // Bus lines number: 1 (default) or 4
    bool securedMode;                                       // Card in Secured mode
    CardType cardType = CardType::None;                     // Card type (R/W, ROM, OTP)
    uint32_t protectedAreaSize;     
    CardSpeedClass speedClass = CardSpeedClass::None;       // Card speed class
    uint8_t perfomanceMove;
    AllocateUnitSize aus = AllocateUnitSize::None;          // Allocated cell size
    uint16_t eraseSize;                                     // The number of cells that can be cleared in one operation
    uint8_t  eraseTimeout;                                  // Erasing timeout (in seconds, for any cells count), 0 if not supported
    uint8_t  eraseOffset;
    UHSMode  uhsSpeedGrade = UHSMode::None;                 // Speed mode for Ultra High Speed cards
    AllocateUnitSize uhsAus = AllocateUnitSize::None;       // The size of allocated cell for Ultra High Speed cards
    CardVideoClass videoSpeed = CardVideoClass::NotSupported;
};

/**
 * @brief SCR (Card Configuration register) structure
 */
struct SCR
{
    PhysicSpecification spec = PhysicSpecification::None;   // Hardware specification
    bool stateAfterErase;                                   // Bit state after erasing (1 or 0)
    SdSecurity security = SdSecurity::None;                 // Security version
    bool support1bitBus = false;                            // 1-wire bus support
    bool support4bitBus = false;                            // 4-wire bus support
    bool supportACMD53_54 = false;                          // ACMD53/54 commands support: Secure send/receive
    bool supportCMD58_59 = false;                           // CMD58/59 commands support: Extension Register Multi Block
    bool supportCMD48_49 = false;                           // CMD58/59 commands support: Extension Register Single Block
    bool supportCMD23 = false;                              // CMD23 command support: Set Block Count
    bool supportCMD20 = false;                              // CMD20 command support: Speed Class Control
};

/**
 * @brief CSD structures for different SD cards versions
 */
#pragma pack (push, 1)
struct SdCsdReg1    // SD card version 1.0 (standart size up to 2GB)
{
    uint8_t MaxDataTransferSpeed : 8;
    uint8_t DataReadAccessTime2 : 8;    // In clock cycles (*100)
    uint8_t DataReadAccessTime1 : 8;
    uint8_t Reserved1 : 6;
    uint8_t StructVers : 2;             // Struct version
    uint16_t DevSize1 : 10;             // Device size part 1
    uint8_t Reserved2 : 2;
    uint8_t DsReg : 1;                  // DS register realised
    uint8_t ReadBlockMisalign : 1;      // Reading block misalignment
    uint8_t WriteBlockMisalign : 1;     // Writing block misalignment
    uint8_t PartialBlockRead : 1;       // Partial block reading support
    uint8_t MaxReadDataBlockLen : 4;
    uint16_t CardCmdClasses : 12;
    uint8_t WriteProtectGroupSize : 7;
    uint8_t EraseSectorSize : 7;
    uint8_t EraseSingleBlockEn : 1;     // The ability to erase a single block
    uint8_t DevSizeMult : 3;            // Device size Multiplier
    uint8_t WriteCurMax : 3;            // Max writing current at maxi supply voltage
    uint8_t WriteCurMin : 3;            // Max writing current at mini supply voltage
    uint8_t ReadCurMax : 3;             // Max reading current at maxi supply voltage
    uint8_t ReadCurMin : 3;             // Max reading current at mini supply voltage
    uint8_t DevSize2 : 2;               // Device size part 2
    uint8_t Reserved7 : 1;
    uint8_t Crc : 7;
    uint8_t Reserved6 : 1;
    uint8_t WProtectInPowerCycle : 1;   // Write protect until Power Cycle
    uint8_t FileFormat : 2;
    uint8_t TemporaryWriteProtect : 1;
    uint8_t PermanentWriteProtect : 1;
    uint8_t CopyFlag : 1;
    uint8_t FileFormatGroup : 1;
    uint8_t Reserved5 : 5;
    uint8_t PartialBlockWrite : 1;       // Partial block writing support
    uint8_t MaxWriteDataBlockLen : 4;
    uint8_t WriteSpeedFactor : 3;
    uint8_t Reserved4 : 2;
    uint8_t WriteProtectGroupEn : 1;
};
#pragma pack (pop)

#pragma pack (push, 1)
struct SdCsdReg2    // SD card version 2.0 (high 2GB-32GB and extended 32GB-2TB capacity)
{
    uint8_t MaxDataTransferSpeed : 8;
    uint8_t DataReadAccessTime2 : 8;    // In clock cycles (*100)
    uint8_t DataReadAccessTime1 : 8;
    uint8_t Reserved1 : 6;
    uint8_t StructVers : 2;             // Struct version
    uint16_t DevSize1 : 6;              // Device size часть 1
    uint8_t Reserved2 : 6;
    uint8_t DsReg : 1;                  // DS register realised
    uint8_t ReadBlockMisalign : 1;      // Reading block misalignment
    uint8_t WriteBlockMisalign : 1;     // Writing block misalignment
    uint8_t PartialBlockRead : 1;       // Partial block reading support
    uint8_t MaxReadDataBlockLen : 4;
    uint16_t CardCmdClasses : 12;
    uint8_t WriteProtectGroupSize : 7;
    uint8_t EraseSectorSize : 7;
    uint8_t EraseSingleBlockEn : 1;     // The ability to erase a single block
    uint8_t Reserved3 : 1;
    uint16_t DevSize2 : 16;             // Device size часть 2
    uint8_t Reserved7 : 1;
    uint8_t Crc : 7;
    uint8_t Reserved6 : 1;
    uint8_t WProtectInPowerCycle : 1;   // Write protect until Power Cycle
    uint8_t FileFormat : 2;
    uint8_t TemporaryWriteProtect : 1;
    uint8_t PermanentWriteProtect : 1;
    uint8_t CopyFlag : 1;
    uint8_t FileFormatGroup : 1;
    uint8_t Reserved5 : 5;
    uint8_t PartialBlockWrite : 1;       // Partial block writing support
    uint8_t MaxWriteDataBlockLen : 4;
    uint8_t WriteSpeedFactor : 3;
    uint8_t Reserved4 : 2;
    uint8_t WriteProtectGroupEn : 1;
};
#pragma pack (pop)

/**
 * @brief CSD structure for eMMC chip
 */
#pragma pack (push, 1)
struct EmmcCsdReg
{
    uint8_t MaxBusClockFreq : 8;
    uint8_t DataReadAccessTime2 : 8;    // In clock cycles (*100)
    uint8_t DataReadAccessTime1 : 8;
    uint8_t Reserved1 : 2;
    uint8_t SysSpecVers : 4;
    uint8_t StructVers : 2;             // Struct version
    uint16_t DevSize1 : 10;             // Device size part 1 (not used for capacity > 2GB)
    uint8_t Reserved2 : 2;
    uint8_t DsReg : 1;                  // DS register realised
    uint8_t ReadBlockMisalign : 1;      // Reading block misalignment
    uint8_t WriteBlockMisalign : 1;     // Writing block misalignment
    uint8_t PartialBlockRead : 1;       // Partial block reading support
    uint8_t MaxReadDataBlockLen : 4;
    uint16_t CardCmdClasses : 12;
    uint8_t WriteProtectGroupSize : 5;
    uint8_t EraseGroupSizeMult : 5;
    uint8_t EraseGroupSize : 5;
    uint8_t DevSizeMult : 3;            // Device size Multiplier
    uint8_t WriteCurMax : 3;            // Max writing current at maxi supply voltage
    uint8_t WriteCurMin : 3;            // Max writing current at mini supply voltage
    uint8_t ReadCurMax : 3;             // Max reading current at maxi supply voltage
    uint8_t ReadCurMin : 3;             // Max reading current at mini supply voltage
    uint8_t DevSize2 : 2;               // Device size part 2 (not used for capacity > 2GB)
    uint8_t Reserved5 : 1;
    uint8_t Crc : 7;
    uint8_t Ecc : 2;
    uint8_t FileFormat : 2;
    uint8_t TemporaryWriteProtect : 1;
    uint8_t PermanentWriteProtect : 1;
    uint8_t CopyFlag : 1;
    uint8_t FileFormatGroup : 1;
    uint8_t ContentProtectApp : 1;
    uint8_t Reserved4 : 4;
    uint8_t PartialBlockWrite : 1;       // Partial block writing support
    uint8_t MaxWriteDataBlockLen : 4;
    uint8_t WriteSpeedFactor : 3;
    uint8_t DefaultECC : 2;
    uint8_t WriteProtectGroupEn : 1;
};
#pragma pack (pop)


/**
 * @brief Response type for SD card
 */
enum class ResponseType : uint8_t
{
    None,   // no response, check only sending the command.
    R1,     // Present, CRC, opcode (48 bit)
    R2,     // Present, long response, CRC (136 bit)
    R3,     // Present (48 bit)
    R6,     // Only for operations with RCA
    R7,     // Only for checking the compatibility of the card with v2.0
};

enum class ResponseSize : uint8_t
{
    NoResponse = 0,
    ShortResponse = 1,
    LongResponse = 3
};

}   // namespace sdmmc
}   // namespace driver
