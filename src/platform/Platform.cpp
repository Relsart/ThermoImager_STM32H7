
#include "Platform.h"
#include "Terminal.h"
#include "etl/utility.h"

namespace platform {
using namespace driver;
namespace args = std::placeholders;

Mcu mcu;
Peripheral peripheral;

/*
 *    * * * NUCLEO_H7 board periphery: * * *
 *
 *  - Green led on PB0
 *  - Red led on PB14
 *  - Yellow led on PE1
 *  - Button on PC13
 *  
 *    * * * DevEBox STM32H743VGT6 little black board periphery: * * *
 *
 *  - Led on PA1
 *  - Button K1 on PE3
 *  - Button K2 on PC5
 *  - MicroSD/MMC card slot connected to SDMMC1:
 *    D0 on PC8
 *    D1 on PC9
 *    D2 on PC10
 *    D3 on PC11
 *    CK on PC12
 *    CMD on PD2
 * 
 *    * * * ZL-10 STM32H743VGT6 black board with CAN, RS-485 and RS-232 periphery: * * *
 * 
 *  - led (D2) on PE1
 *  - Red led (D3) on PE0
 *  - Button K2 on PC4
 *  - Button K3 on PC5
 *  - CAN bus (on chip) CAN1:
 *    CAN RX on PA11
 *    CAN TX on PA12
 *
 */

Mcu::Mcu () : console(USART2, 128),
              screenSpi(SPI1),
              boardButton(GPIOE, 3, gpio::PinType::Input, gpio::Pull::PullUp, gpio::ExtiFront::FailingFront),
              boardLed(GPIOA, 1, gpio::PinType::Out_pushpull, gpio::Pull::NoPull),
              screenBlk(GPIOB, 0, gpio::PinType::Out_pushpull, gpio::Pull::NoPull),
              screenReset(GPIOB, 14, gpio::PinType::Out_pushpull, gpio::Pull::PullUp),
              screenDc(GPIOB, 1, gpio::PinType::Out_pushpull, gpio::Pull::NoPull),
              screenCS(GPIOB, 12, gpio::PinType::Out_pushpull, gpio::Pull::NoPull),
              thermoSensorPwr(GPIOC, 10, gpio::PinType::Out_pushpull, gpio::Pull::NoPull),
              ledBlinker(boardLed),
              i2cPort(I2C2)
{}

void Mcu::init ()
{
    /**
     * @brief ================ initialization of SYSTICK timer: ================
     */
    SysTick_Config (SystemCoreClock / 1000);   // System ticker interrupt config to one millisecond
    __NVIC_SetPriority(SysTick_IRQn, 0);       // SysTick Priopity mast be the HIGHEST for RTOS! 
    __NVIC_EnableIRQ (SysTick_IRQn);           // Enable the System Tick Interrupt
    CRCInit ();                                // Hardware CRC calculator clocking on

    /**
     * @brief ====================== GPIO initialization: ======================
     */
    {
        using namespace driver::gpio;
        /*  Terminal pins: (it's better to use USART2 for NUCLEO H7 board on CN9 connector) */
        Pin::config (GPIOD, 5, PinType::Alt_pushpull, Pull::NoPull, PinSpeed::High, AltFuncNumber::AfUart2);     // USART2 TX
        Pin::config (GPIOD, 6, PinType::Alt_pushpull, Pull::NoPull, PinSpeed::High, AltFuncNumber::AfUart2);     // USART2 RX
        /*  I2C  */
        Pin::config (GPIOF, 1, PinType::Alt_opendrain, Pull::NoPull, PinSpeed::VeryHigh, AltFuncNumber::I2C_2);  // I2C_2 SCL
        Pin::config (GPIOF, 0, PinType::Alt_opendrain, Pull::NoPull, PinSpeed::VeryHigh, AltFuncNumber::I2C_2);  // I2C_2 SDA
        /*  SPI  */
        Pin::config (GPIOA, 5, PinType::Alt_pushpull, Pull::PullUp, PinSpeed::VeryHigh, AltFuncNumber::AfSpi1);  // SPI1_SCK
        Pin::config (GPIOA, 7, PinType::Alt_pushpull, Pull::NoPull, PinSpeed::VeryHigh, AltFuncNumber::AfSpi1);  // SPI1_MOSI

        /*  SDMMC for microSD memory card  */
        #ifdef SD_CARD_ON_DEVEBOX
        Pin::config (GPIOC, 8,  PinType::Alt_pushpull, Pull::PullUp, PinSpeed::High, AltFuncNumber::SDMMC1_D0);
        Pin::config (GPIOC, 9,  PinType::Alt_pushpull, Pull::PullUp, PinSpeed::High, AltFuncNumber::SDMMC1_D1);
        Pin::config (GPIOC, 10, PinType::Alt_pushpull, Pull::PullUp, PinSpeed::High, AltFuncNumber::SDMMC1_D2);
        Pin::config (GPIOC, 11, PinType::Alt_pushpull, Pull::PullUp, PinSpeed::High, AltFuncNumber::SDMMC1_D3);
        Pin::config (GPIOC, 12, PinType::Alt_pushpull, Pull::NoPull, PinSpeed::High, AltFuncNumber::SDMMC1_CK);
        Pin::config (GPIOD, 2,  PinType::Alt_pushpull, Pull::PullUp, PinSpeed::High, AltFuncNumber::SDMMC1_CMD);
        #endif
    }   // using namespace driver::gpio;

    /**
     * @brief ==================== Terminal initialization: ====================
     */
    console.init(921600);
    console.setSlotRx(&(console::Terminal::getInstance()));
    console.onTxStream(&logStream);
    console.setTxMode(uart::Mode::Dma);
    console.setNvicPriority(4);
    
    Log::setTransmitter(Log::DataSender::create<uart::Port, &uart::Port::transmit>(console));
    Log::setSysTimeGetter(Log::GetSysTime::create<getMsTicks>());
    Log::setPortChecking(Log::IsPortAvailable::create<uart::Port, &uart::Port::dmaTxIsBusy>(console));
    Log::groupOn(lmSystem);
    Log::groupOn(lmTask);
    Log(lmSystem, Info) << "Console initialised";

    /**
     * @brief ======================= SPI initialization: =======================
     */
    {
        using namespace driver::spi;
        spi::Config spiconf = 
        {
            .mode = DeviceMode::Master,        // Master mode
            .firstBit = FirstBit::Msb,         // Order MSB first
            .dirMode = WorkMethod::TwoLines,   // Full duplex
            .speedDiv = FreqPrescaler::Div8,   // SCK frequency
            .nssMode = CsMode::Soft,           // NSS signal software control
            .sckPol = Level::High,             // SCK low level in idle
            .sckPhase = Phase::Second,         // Signal capture at first front of SCK
            .masterRxAutoSuspend = true,
            .crcEnable = false,                // CRC disabled
            .fifoThreshold = 1
        };
        screenSpi.init(spiconf);
        screenSpi.onDmaTxStream(&spiStream);
        uint32_t spiFrq = screenSpi.getSckFreq();
        Log(lmSystem, Info) << "SPI1 speed = " << screenSpi.getSckFreq() << " Hz";
    }   // using namespace driver::gpio;

    /**
     * @brief ======================= I2C initialization: =======================
     */
    i2cPort.init(I2C::Speed::Fast400kHz);
    i2cPort.onRxStream(&i2cStream);
    
    /**
     * @brief ========== GPIO pin diagnostic against re-initialization: ==========
     */
    gpio::Pin::pinConfigDiagnostic();
}


Peripheral::Peripheral() :
    m_Mcu(&mcu),
    m_thermoMatrixTask(&m_Mcu->i2cPort, 0x33),
    m_displayPeriph(m_spiDisplayMutex),
    m_display(m_displayPeriph, 320, 480),
    m_thermoDisplayTask(m_display)
{}

bool Peripheral::init()
{
    /* Display middleware periphery driver initialisation: */
    using namespace graphic::st77xx;
    m_displayPeriph.setSpiTx8Pio(PeripheryDriver::SpiTx8b::create<spi::Spi, &spi::Spi::transmit8>(m_Mcu->screenSpi));
    m_displayPeriph.setSpiTx16Pio(PeripheryDriver::SpiTx16b::create<spi::Spi, &spi::Spi::transmit16>(m_Mcu->screenSpi));
    m_displayPeriph.setSpiTx16Dma(PeripheryDriver::SpiTx16b::create<spi::Spi, &spi::Spi::transmit16viaDMA>(m_Mcu->screenSpi));
    m_displayPeriph.setDelayMs(PeripheryDriver::DelayMs::create<DwtTimer, &DwtTimer::delayMs>(DwtTimer::getInstance()));
    m_displayPeriph.setRstPinControl(PeripheryDriver::PinControl::create<gpio::Pin, &gpio::Pin::set>(m_Mcu->screenReset));
    m_displayPeriph.setDcPinControl(PeripheryDriver::PinControl::create<gpio::Pin, &gpio::Pin::set>(m_Mcu->screenDc));
    m_displayPeriph.setBlkPinControl(PeripheryDriver::PinControl::create<gpio::Pin, &gpio::Pin::set>(m_Mcu->screenBlk));
    m_displayPeriph.setCsPinControl(PeripheryDriver::PinControl::create<gpio::Pin, &gpio::Pin::set>(m_Mcu->screenCS));
    m_Mcu->screenSpi.xferCmpltSubscribe(&m_displayPeriph);
    m_displayPeriph.setSpiXferFinish(PeripheryDriver::XferFinish::create<TermoScreen, &TermoScreen::spiXferFinishHandler>(m_thermoDisplayTask));

    /* ST7796 Display initialisation: */
    graphic::st77xx::Config dicplayConf;
    dicplayConf.chip = graphic::st77xx::ChipType::St7796;
    //dicplayConf.rotate90 = true;
    dicplayConf.colOrder = graphic::st77xx::ColourOrder::Bgr;
    if (m_display.init(dicplayConf))
    {
        m_display.clearScreen();
        Log(lmSystem, Info) << "Display init - OK";
    }
    else
        Log(lmSystem, Error) << "Display init - FAILED";

    /* Thermo picture builder initialisation: */
    m_thermoDisplayTask.tasksInit();

    /* MLX90640 Thermomatrix task initialisation: */
    if (m_thermoMatrixTask.init(thermomatrix::Mlx90640::RefreshRate::Rate8HZ))
        m_thermoMatrixTask.setMeasConsumer(&m_thermoDisplayTask);
    else
        Log(lmSystem, Error) << "MLX90640 init - FAILED";

    return true;
}

}   // namespace platform
