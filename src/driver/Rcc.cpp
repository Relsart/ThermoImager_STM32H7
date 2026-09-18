#include "Rcc.h"
#include "driver/Dwt.h"

namespace driver {

/**
 * @brief Settings for dividers and multipliers is here
 * @attention ACHTUNG! Do not forget to set quartz frequency (HSE_VALUE) in main CMakeLists.txt!
 */

#if HSE_VALUE == 8000000
/*  Settings for external 8 MHz quartz:
    SysClock = 64 MHz
    PLL1 out (P) = 64 MHz
    PLL1 out (Q) = 32 MHz
    PLL1 out (R) = 128 MHz  */
/*#define DIVM_1  2
#define DIVN_1  64
#define DIVP_1  4
#define DIVQ_1  8
#define DIVR_1  2
#define FRACN_1 0
#define HPRE_Prescaler  RCC_D1CFGR_HPRE_DIV1
#define D2PRE RCC_D2CFGR_D2PPRE1_DIV1*/

/*  Settings for external 8 MHz quartz:
    SysClock = 400 MHz
    PLL1 out (P) = 400 MHz
    PLL1 out (Q) = 200 MHz
    PLL1 out (R) = 400 MHz  */
#define DIVM_1  2
#define DIVN_1  200
#define DIVP_1  2
#define DIVQ_1  4   //(CHECK! For Display SPI)
#define DIVR_1  2
#define FRACN_1 0
#define HPRE_Prescaler  RCC_D1CFGR_HPRE_DIV4
#define D2PRE RCC_D2CFGR_D2PPRE1_DIV1

/*  PLL3: for I2C (50MHz max recommended): 
    PLL3 out (P) = 100 MHz
    PLL3 out (Q) = 100 MHz
    PLL3 out (R) = 50 MHz   */
#define DIVM_3  4
#define DIVN_3  100
#define DIVP_3  2
#define DIVQ_3  2
#define DIVR_3  4
#define FRACN_3 0


#elif HSE_VALUE == 25000000
/*  Settings for external 25 MHz quartz:  */
/*  SysClock = 400 MHz;  PLL1 out (P, Q, R) = 100 MHz  */
#define DIVM_1  5
#define DIVN_1  160
#define DIVP_1  2
#define DIVQ_1  8
#define DIVR_1  8
#define FRACN_1 0
#define HPRE_Prescaler  RCC_D1CFGR_HPRE_DIV4
#define D2PRE RCC_D2CFGR_D2PPRE1_DIV1
/*  SysClock = 100 MHz;  PLL1 out (P, Q, R) = 100 MHz  */
// #define DIVM_1  8
// #define DIVN_1  64
// #define DIVP_1  2
// #define DIVQ_1  2
// #define DIVR_1  2
// #define FRACN_1 0
// #define HPRE_Prescaler  RCC_D1CFGR_HPRE_DIV1
// #define D2PRE RCC_D2CFGR_D2PPRE1_DIV2
#endif

bool Rcc::extClockConfig()
{
    __disable_irq();

    // External quartz enabling:
    SET_BIT(RCC->CR, RCC_CR_HSEON);                        // Enable HSE external clock and waiting for enabling
    driver::DwtTimer::TimeoutData param = driver::DwtTimer::getInstance().timeoutInit(1000);
    while (READ_BIT(RCC->CR, RCC_CR_HSERDY) == 0)
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param)) return false;
    }

    // PLL settings:
    CLEAR_BIT(RCC->CR, RCC_CR_PLL1ON);                     // Disable PLL1 and wait for disabling
    param = driver::DwtTimer::getInstance().timeoutInit(1000);
    while (READ_BIT (RCC->CR, RCC_CR_PLL1RDY) != 0)
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param)) return false;
    }

    CLEAR_BIT(RCC->CR, RCC_CR_PLL2ON);                     // Disable PLL2 and wait for disabling
    param = driver::DwtTimer::getInstance().timeoutInit(1000);
    while (READ_BIT(RCC->CR, RCC_CR_PLL2RDY) != 0)
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param)) return false;
    }

    CLEAR_BIT(RCC->CR, RCC_CR_PLL3ON);                     // Disable PLL3 and wait for disabling
    param = driver::DwtTimer::getInstance().timeoutInit(1000);
    while (READ_BIT(RCC->CR, RCC_CR_PLL3RDY) != 0)
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param)) return false;
    }

    SET_BIT(RCC->PLLCKSELR, RCC_PLLCKSELR_PLLSRC_HSE);                                         // Clock source for PLL = external quartz
    /* PLL1 configuration: */
    MODIFY_REG(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM1_Msk, DIVM_1 << RCC_PLLCKSELR_DIVM1_Pos);    // DIVM1 divider
    MODIFY_REG(RCC->PLL1DIVR, RCC_PLL1DIVR_N1_Msk, (DIVN_1 - 1) << RCC_PLL1DIVR_N1_Pos);       // DIVN1 multiplier (the value in the register - 1)
    MODIFY_REG(RCC->PLL1DIVR, RCC_PLL1DIVR_P1_Msk, (DIVP_1 - 1) << RCC_PLL1DIVR_P1_Pos);       // DIVP1 divider (the value in the register - 1)
    MODIFY_REG(RCC->PLL1DIVR, RCC_PLL1DIVR_Q1_Msk, (DIVQ_1 - 1) << RCC_PLL1DIVR_Q1_Pos);       // DIVQ1 divider (the value in the register - 1)
    MODIFY_REG(RCC->PLL1DIVR, RCC_PLL1DIVR_R1_Msk, (DIVR_1 - 1) << RCC_PLL1DIVR_R1_Pos);       // DIVR1 divider (the value in the register - 1)
    CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL1FRACEN);                        // Disable fracn divider
    CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL1VCOSEL);                        // Standart frequency range (192..836 MHz)
    MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLL1RGE, RCC_PLLCFGR_PLL1RGE_2);   // PLL1 output frequency range 4..8 MHz
    if (FRACN_1 <= 8191)
        MODIFY_REG(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACN1_Msk , FRACN_1 << RCC_PLL1FRACR_FRACN1_Pos);    // FRACN precise frequency adjustment
    SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_DIVP1EN);                    // Enable P divider in PLL1
    SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_DIVQ1EN);                    // Enable Q divider in PLL1
    SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_DIVR1EN);                    // Enable R divider in PLL1
    SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL1FRACEN);                 // Enable fracn divider in PLL1

    MODIFY_REG(RCC->D1CFGR, RCC_D1CFGR_HPRE , HPRE_Prescaler);     // HPRE divider

    /* PLL3 configuration: */
    MODIFY_REG(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM3_Msk, DIVM_3 << RCC_PLLCKSELR_DIVM3_Pos);    // DIVM3 divider
    MODIFY_REG(RCC->PLL3DIVR, RCC_PLL3DIVR_N3_Msk, (DIVN_3 - 1) << RCC_PLL3DIVR_N3_Pos);       // DIVN3 multiplier (the value in the register - 1)
    MODIFY_REG(RCC->PLL3DIVR, RCC_PLL3DIVR_P3_Msk, (DIVP_3 - 1) << RCC_PLL3DIVR_P3_Pos);       // DIVP3 divider (the value in the register - 1)
    MODIFY_REG(RCC->PLL3DIVR, RCC_PLL3DIVR_Q3_Msk, (DIVQ_3 - 1) << RCC_PLL3DIVR_Q3_Pos);       // DIVQ3 divider (the value in the register - 1)
    MODIFY_REG(RCC->PLL3DIVR, RCC_PLL3DIVR_R3_Msk, (DIVR_3 - 1) << RCC_PLL3DIVR_R3_Pos);       // DIVR3 divider (the value in the register - 1)
    CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL3FRACEN);                        // Disable fracn divider
    CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL3VCOSEL);                        // Standart frequency range (192..836 MHz)
    MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLL3RGE, RCC_PLLCFGR_PLL3RGE_2);   // PLL3 output frequency range 4..8 MHz
    if (FRACN_3 <= 8191)
        MODIFY_REG(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACN1_Msk , FRACN_1 << RCC_PLL1FRACR_FRACN1_Pos);    // FRACN precise frequency adjustment
    SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_DIVP3EN);                    // Enable P divider in PLL3
    SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_DIVQ3EN);                    // Enable Q divider in PLL3
    SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_DIVR3EN);                    // Enable R divider in PLL3
    SET_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLL3FRACEN);                 // Enable fracn divider in PLL3

    // Starting PLL1:
    SET_BIT(RCC->CR, RCC_CR_PLL1ON);
    param = driver::DwtTimer::getInstance().timeoutInit(1000);
    while (READ_BIT(RCC->CR, RCC_CR_PLL1RDY) == 0)
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param)) return false;
    }
    // Starting PLL3:
    SET_BIT(RCC->CR, RCC_CR_PLL3ON);
    param = driver::DwtTimer::getInstance().timeoutInit(1000);
    while (READ_BIT(RCC->CR, RCC_CR_PLL3RDY) == 0)
    {
        if (driver::DwtTimer::getInstance().checkTimeout(param)) return false;
    }

    // System Clock Mux switch to PLL1:
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_0 | RCC_CFGR_SW_1);
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1) {};

    // Setting dividers after System Clock Mux D1CPRE and HPRE:
    MODIFY_REG(RCC->D2CFGR , RCC_D2CFGR_D2PPRE1, D2PRE);

    // FDCAN clocking by PLL1_Q:
    MODIFY_REG(RCC->D2CCIP1R , RCC_D2CCIP1R_FDCANSEL, RCC_D2CCIP1R_FDCANSEL_0);

    // USB clocking by PLL1_Q
    MODIFY_REG(RCC->D2CCIP2R, RCC_D2CCIP2R_USBSEL, RCC_D2CCIP2R_USBSEL_0);

    // I2C clocking by PLL3_R
    MODIFY_REG(RCC->D2CCIP2R, RCC_D2CCIP2R_I2C123SEL, RCC_D2CCIP2R_I2C123SEL_0);

    // SystemCoreClock updating:
    SystemCoreClockUpdate();
    return true;
}

Rcc& Rcc::getInstance()
{
    static Rcc self;
    return self;
}

uint32_t Rcc::getHsiClock()
{
    uint32_t hsiDiv = (READ_BIT(RCC->CR, RCC_CR_HSIDIV)) >> RCC_CR_HSIDIV_Pos;

    switch (hsiDiv)
    {
    case 0: return 64000000;
    case 1: return 32000000;
    case 2: return 16000000;
    case 3: return 8000000;
    }
    return 0;
}

uint32_t Rcc::getPllFreqHz(Pll pll, PllOutDivider presc)
{
    uint32_t freq = 0;
    uint32_t pllDivReg = 0;
    uint32_t divm = 0;
    uint32_t divn = 0;
    uint32_t fracn = 0;

    // Get base frequency at all PLLs inputs:
    uint32_t selector = READ_BIT(RCC->PLLCKSELR, RCC_PLLCKSELR_PLLSRC);

    switch (selector)
    {
    case 0:     // internal RC HSI clocking
        freq = getHsiClock();
        break;
    case 1:     // internal RC CSI clocking
        freq = csiFreqHz;
        break;
    case 2:     // External quartz clocking
        freq = HSE_VALUE;
        break;
    case 3:     // No clocking
        freq = 0;
        break;
    }

    // Get DIVM, DIVN и FRACN values:
    switch (pll)
    {
    case Pll::PLL1:
        divm = (READ_BIT(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM1)) >> RCC_PLLCKSELR_DIVM1_Pos;
        pllDivReg = RCC->PLL1DIVR;
        fracn = (READ_BIT(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACN1)) >> RCC_PLL1FRACR_FRACN1_Pos;
        break;
    case Pll::PLL2:
        divm = (READ_BIT(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM2)) >> RCC_PLLCKSELR_DIVM2_Pos;
        pllDivReg = RCC->PLL2DIVR;
        fracn = (READ_BIT(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACN2)) >> RCC_PLL2FRACR_FRACN2_Pos;
        break;
    case Pll::PLL3:
        divm = (READ_BIT(RCC->PLLCKSELR, RCC_PLLCKSELR_DIVM3)) >> RCC_PLLCKSELR_DIVM3_Pos;
        pllDivReg = RCC->PLL3DIVR;
        fracn = (READ_BIT(RCC->PLL3FRACR, RCC_PLL3FRACR_FRACN3)) >> RCC_PLL3FRACR_FRACN3_Pos;
        break;
    }

    if (divm == 0)
        return 0;   // PLL disable

    divn = (READ_BIT(pllDivReg, RCC_PLL1DIVR_N1)) + 1;   // Mask and positions for PLL2 and PLL3 are the same
    
    // Get frequency before dividers Q, P, R with fine adjustment FRACN:
    float mult = static_cast<float>(divn) + (static_cast<float>(fracn) / 8192.0);   // 8192 = 2^13, formula in Reference Manual, registers RCC_PLLxFRACR description
    freq = (freq / divm);
    freq = static_cast<uint32_t>((float)freq * mult);

    // Get frequency after DIVP, DIVQ или DIVR, at PLL output:
    uint32_t div = 0;   // Divider value
    switch (presc)
    {
    case PllOutDivider::P:
        div = ((READ_BIT(pllDivReg, RCC_PLL1DIVR_P1)) >> RCC_PLL1DIVR_P1_Pos) + 1;
        break;
    case PllOutDivider::Q:
        div = ((READ_BIT(pllDivReg, RCC_PLL1DIVR_Q1)) >> RCC_PLL1DIVR_Q1_Pos) + 1;
        break;
    case PllOutDivider::R:
        div = ((READ_BIT(pllDivReg, RCC_PLL1DIVR_R1)) >> RCC_PLL1DIVR_R1_Pos) + 1;
        break;
    }

    return (div != 0) ? freq/div : 0;   // Dividing by 0 checking
}

uint32_t Rcc::getPerFreq()
{
    uint32_t ckPerSel = ((READ_BIT(RCC->D1CCIPR, RCC_D1CCIPR_CKPERSEL)) >> RCC_D1CCIPR_CKPERSEL_Pos);
    switch (ckPerSel)
    {
    case 0:
        return getHsiClock();
    case 1:
        return csiFreqHz;
    case 2:
        return HSE_VALUE;    
    default:
        return 0;
    }
}

uint32_t Rcc::getPeriphClock(PeriphBus bus)
{
    // SystemCoreClock contains frequency, allready divided by D1CPRE
    uint32_t freq = SystemCoreClock;

    // Get frequency after HPRE divider:
    uint32_t divider = RCC->D1CFGR & RCC_D1CFGR_HPRE;
    uint8_t divTable[]{1, 2, 3, 4, 6, 7, 8, 9};
    uint8_t divShift = 0;

    if ((divider & 0b1000) != 0)    // If less than 0b1000 no division
    {
        divider &= 0b111;
        freq = freq >> divTable[divider];   // Dividing by shifting
    }

    // Get divider for each bus:
    switch (bus)
    {
    case PeriphBus::Ahb12:
        divider = 0;
        break;
    case PeriphBus::Apb1:
        divider = (RCC->D2CFGR & RCC_D2CFGR_D2PPRE1) >> RCC_D2CFGR_D2PPRE1_Pos;
        break;
    case PeriphBus::Apb2:
        divider = (RCC->D2CFGR & RCC_D2CFGR_D2PPRE2) >> RCC_D2CFGR_D2PPRE2_Pos;
        break;
    case PeriphBus::Apb3:
        divider = (RCC->D1CFGR & RCC_D1CFGR_D1PPRE) >> RCC_D1CFGR_D1PPRE_Pos;
        break;
    case PeriphBus::Apb4:
        divider = (RCC->D3CFGR & RCC_D3CFGR_D3PPRE) >> RCC_D3CFGR_D3PPRE_Pos;
        break;
    }

    // Calc frequency after divider:
    if ((divider & 0b100) != 0)
    {
        divider &= 0b11;
        freq = freq >> divTable[divider];
    }

    return freq;
}

uint32_t Rcc::getFreq(uint32_t deviceAddr)
{
    uint32_t freq = 0;
    switch (deviceAddr)
    {
    case USART2_BASE:
    case USART3_BASE:
    case UART4_BASE:
    case UART5_BASE:
    case UART7_BASE:
    case UART8_BASE:
        {
            uint32_t prescaler = D1CorePrescTable[(RCC->D1CFGR & RCC_D1CFGR_HPRE)>> POSITION_VAL(RCC_D1CFGR_HPRE_0)];
            prescaler +=  D1CorePrescTable[(RCC->D2CFGR & RCC_D2CFGR_D2PPRE1)>> POSITION_VAL(RCC_D2CFGR_D2PPRE1_0)];
            freq = SystemCoreClock >> prescaler;
        }
        break;
    case USART1_BASE:
    case USART6_BASE:
        {
            uint32_t prescaler = D1CorePrescTable[(RCC->D1CFGR & RCC_D1CFGR_HPRE)>> POSITION_VAL(RCC_D1CFGR_HPRE_0)]; // 0
            prescaler +=  D1CorePrescTable[(RCC->D2CFGR & RCC_D2CFGR_D2PPRE2)>> POSITION_VAL(RCC_D2CFGR_D2PPRE2_0)];  // 0
            freq = SystemCoreClock >> prescaler;
        }
        break;
    case SPI1_BASE:
    case SPI2_BASE:
    case SPI3_BASE:
        {
            uint32_t sel = (READ_BIT(RCC->D2CCIP1R, RCC_D2CCIP1R_SPI123SEL)) >> RCC_D2CCIP1R_SPI123SEL_Pos;
            if (sel == 0)       
                freq = getPllFreqHz(Pll::PLL1, PllOutDivider::Q); // SPI1,2,3 clocking by PLL1_Q
            else if (sel == 1)  
                freq = getPllFreqHz(Pll::PLL2, PllOutDivider::P); // SPI1,2,3 clocking by PLL2_P
            else if (sel == 2)  
                freq = getPllFreqHz(Pll::PLL3, PllOutDivider::P); // SPI1,2,3 clocking by PLL3_P
            else if (sel == 3)  
                freq = 0;              // SPI1,2,3 clocking by extern signal on I2S_CKIN
            else if (sel == 4)
                freq = getPerFreq();   // SPI1,2,3 clocking by PER selector
            else                       // SPI1,2,3 clock disable
                freq = 0;
        }
        break;
    case SPI4_BASE:
    case SPI5_BASE:
        {
            uint32_t sel = (READ_BIT(RCC->D2CCIP1R, RCC_D2CCIP1R_SPI45SEL)) >> RCC_D2CCIP1R_SPI45SEL_Pos;
            if (sel == 0)       
                freq = getPeriphClock(PeriphBus::Apb2);           // SPI4,5 clocking by APB2 bus
            else if (sel == 1)  
                freq = getPllFreqHz(Pll::PLL2, PllOutDivider::Q); // SPI4,5 clocking by PLL2_Q
            else if (sel == 2)  
                freq = getPllFreqHz(Pll::PLL3, PllOutDivider::Q); // SPI4,5 clocking by PLL3_Q
            else if (sel == 3)  
                freq = getHsiClock();  // SPI4,5 clocking by internal HSI generator
            else if (sel == 4)
                freq = csiFreqHz;      // SPI4,5 clocking by internal CSI generator (4MHz)
            else if (sel == 5)
                freq = HSE_VALUE;      // SPI4,5 clocking by external quartz
            else                       // SPI4,5 clocking disable
                freq = 0;
        }
        break;
        case SPI6_BASE:
        {
            uint32_t sel = (READ_BIT(RCC->D3CCIPR, RCC_D3CCIPR_SPI6SEL)) >> RCC_D3CCIPR_SPI6SEL_Pos;
            if (sel == 0)       
                freq = getPeriphClock(PeriphBus::Apb4);           // SPI6 clocking by APB4 bus
            else if (sel == 1)
                freq = getPllFreqHz(Pll::PLL2, PllOutDivider::Q); // SPI6 clocking by PLL2_Q
            else if (sel == 2)
                freq = getPllFreqHz(Pll::PLL3, PllOutDivider::Q); // SPI6 clocking by PLL3_Q
            else if (sel == 3)
                freq = getHsiClock();   // SPI6 clocking by internal HSI generator
            else if (sel == 4)
                freq = csiFreqHz;       // SPI6 clocking by internal CSI generator (4MHz)
            else if (sel == 5)
                freq = HSE_VALUE;       // SPI6 clocking by external quartz
            else                        // SPI6 clocking disable
                freq = 0;
        }
        break;
        case SDMMC1_BASE:
        case SDMMC2_BASE:
            {
                if (READ_BIT(RCC->D1CCIPR, RCC_D1CCIPR_SDMMCSEL))
                    freq = getPllFreqHz(Pll::PLL2, PllOutDivider::R);
                else
                    freq = getPllFreqHz(Pll::PLL1, PllOutDivider::Q);
            }
            break;
        case I2C1_BASE:
        case I2C2_BASE:
        case I2C3_BASE:
        {
            uint32_t sel = (READ_BIT(RCC->D2CCIP2R, RCC_D2CCIP2R_I2C123SEL)) >> RCC_D2CCIP2R_I2C123SEL_Pos;
            if (sel == 0)
                freq = getPeriphClock(PeriphBus::Apb1);            // Clocking by APB1 bus
            else if (sel == 1)
                freq = getPllFreqHz(Pll::PLL3, PllOutDivider::R);  // Clocking by PLL3_Q
            else if (sel == 2)
                freq = getHsiClock();   // Clocking by internal HSI generator
            else if (sel == 3)
                freq = csiFreqHz;       // Clocking by internal CSI generator (fixed 4MHz)
            else
                freq = 0;
        }
        break;
        case I2C4_BASE:
        {
            uint32_t sel = (READ_BIT(RCC->D3CCIPR, RCC_D3CCIPR_I2C4SEL)) >> RCC_D3CCIPR_I2C4SEL_Pos;
            if (sel == 0)
                freq = getPeriphClock(PeriphBus::Apb4);             // Clocking by APB4 bus
            else if (sel == 1)
                freq = getPllFreqHz(Pll::PLL3, PllOutDivider::R);   // Clocking by PLL3_Q
            else if (sel == 2)
                freq = getHsiClock();  // Clocking by internal HSI generator
            else if (sel == 3)
                freq = csiFreqHz;      // Clocking by internal CSI generator (fixed 4MHz)
            else
                freq = 0;
        }
        break;
        case FDCAN1_BASE:
        case FDCAN2_BASE:
        {
            uint32_t sel = (READ_BIT(RCC->D2CCIP1R, RCC_D2CCIP1R_FDCANSEL)) >> RCC_D2CCIP1R_FDCANSEL_Pos;
            if (sel == 0)
                freq = HSE_VALUE;
            else if (sel == 1)
                freq = getPllFreqHz(Pll::PLL1, PllOutDivider::Q);  // Clocking by PLL1_Q
            else if (sel == 1)
                freq = getPllFreqHz(Pll::PLL2, PllOutDivider::Q);  // Clocking by PLL2_Q
        }
        break;
    }
    return freq;
}

}   // namespace driver