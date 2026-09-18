#include "platform/Platform.h"
#include "Signal.h"

int main(void)
{
    driver::Rcc::getInstance().extClockConfig(); // MCU clock setting
    __enable_irq();
    platform::mcu.init();           // MCU periphery configuration
    platform::peripheral.init();    // External devices and handlers initialisation

    // Main loop enterance:
    #ifndef WITH_RTOS
    Log(lmSystem, Info) << "Starting program main loop";
    EventLoop::getInstance().loop();
    #else
    Log(lmSystem, Info) << "Starting Task Scheduler";
    vTaskStartScheduler();
    for(;;){};
    #endif
}
