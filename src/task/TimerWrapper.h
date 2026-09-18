#pragma once

#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#ifdef WITH_ETL
#include "etl/delegate.h"
#define funct etl::delegate
#else
#include <functional>
#define funct std::function
#endif

namespace rtos
{

class Timer
{
public:
    using Handler = funct<void()>;

    Timer(UBaseType_t timeSetMs) : m_ticksSet(pdMS_TO_TICKS(timeSetMs))
    {
        m_timerHandle = xTimerCreateStatic("Timer", m_ticksSet, pdTRUE, this, timerStaticCallback, &m_timerBuffer);
    }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    void subscribeHandler(Handler clbk)
    {
        m_handler = clbk;
    }

    void start()
    {
        if (m_timerHandle)
            xTimerStart(m_timerHandle, 0); 
    }

    void stop()
    {
        if (m_timerHandle)
            xTimerStop(m_timerHandle, 0);
    }

private:
    TimerHandle_t m_timerHandle;
    StaticTimer_t m_timerBuffer;    // Static buffer for timer
    TickType_t m_ticksSet;          // Time preset for timer (in Ticks)
    Handler m_handler;              // Timers callback

    static void timerStaticCallback(TimerHandle_t xTimer)
    {
        // Extrtact "this" pointer from pvTimerID[1]:
        void* pvID = pvTimerGetTimerID(xTimer);
        Timer* pInstance = static_cast<Timer*>(pvID);
        if (pInstance)
        {
            if (pInstance->m_handler)
                pInstance->m_handler();
        }
    }


};


}  // namespace rtos