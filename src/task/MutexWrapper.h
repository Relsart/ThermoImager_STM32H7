#pragma once

#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

namespace rtos
{

/**
 * @brief Free RTOS Mutex C++ wrapper
 */
class MutexWrapper
{
private:
    SemaphoreHandle_t m_semaphore;
    StaticSemaphore_t m_buffer;     // Static buffer for semaphore

public:
    /**
     * @brief Constructor
     */
    MutexWrapper()
    {
        // Static mutex: without heap/stack usage! 
        m_semaphore = xSemaphoreCreateMutexStatic(&m_buffer);
    }

    /**
     * @brief Copying is forbidden!
     */
    MutexWrapper(const MutexWrapper&) = delete;
    MutexWrapper& operator=(const MutexWrapper&) = delete;

    /**
     * @brief Locking (taking) the mutex
     * @details Unlimited timeout
     */
    void lock()
    {
        xSemaphoreTake(m_semaphore, portMAX_DELAY);
    }

    /**
     * @brief Unlocking (giving, releasing) the muxtex
     */
    void unlock()
    {
        xSemaphoreGive(m_semaphore);
    }

    void unlockFromISR()
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(m_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
};

/**
 * @brief Guard wrapper for simple handlers
 */
class LockGuard
{
private:
    MutexWrapper& m_mutex;

public:
    /**
     * @brief Constructor: lock (take) semaphore at creation
     */
    LockGuard(MutexWrapper& mutex) : m_mutex(mutex)
    {
        m_mutex.lock();
    }

    /**
     * @brief Destructor: unlock (give) semaphore at object dying
     */
    ~LockGuard()
    {
        m_mutex.unlock();
    }
};

}  // namespace rtos
