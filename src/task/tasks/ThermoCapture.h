#pragma once
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "Signal.h"
#include "Mlx90640.h"

namespace driver { class I2C; }
using namespace thermomatrix;

namespace task
{

/**
 * @brief Thermo measurements reading task
 * @details RTOS upper-layered manager for MLX90640 driver
 */
class ThermoMatrixManager : public SlotInterface <uint16_t>
{
public:
    /**
     * @brief Constructor
     * @param [in] i2c low-leveled driver class
     * @param [in] devAddr i2c slave device (MLX90640) address
     */
    ThermoMatrixManager(driver::I2C* i2c, uint16_t devAddr);

    /**
     * @brief Task and device initialisation
     * @param [in] refreshRate refresh frequency
     * @return True == Ok
     */
    bool init(Mlx90640::RefreshRate refreshRate);

    /**
     * @brief Subscribe to ready data (measurements) consumer
     * @param [in] slot Consumer slot
     */
    void setMeasConsumer(SlotInterface<const ThermoArray*>*slot);

    /**
     * @brief Subscribe to sensor power control
     * @param [in] slot Gpio power gontrol slot
     */
    void setPowerCtrl(SlotInterface<bool>*slot);

private:
    /**
     * @brief Working stages
     */
    enum class States
    {
        None,               // Not initialised
        Initialised,        // Initialised successfully, idle mode
        FrameRequested,     // Frame (Pixels) data requested (via DMA stream)
        AuxRequested,       // Aux data requested (via DMA stream)
        CtrlRequested,      // Control register value requested
        DataHandling,       // All data received, handling in process
        Error               // Error state
    } m_state;  // TODO: state checkins and errors handling

    driver::I2C* const m_i2c;       // Periphery driver class (TODO: is needed to store this pointer???)
    const uint16_t m_i2cAddr;       // Sensor I2C slave address
    Signal<bool> m_sensorPower;     // Signal for Mlx90640 sensor power control
    Mlx90640 m_thermoDriver;                        // MLX90640 low-leveled driver
    Signal<const ThermoArray*> m_sendReadyData;     // Signal for sending ready (calculated) thermo data to consumers
    Mlx90640::RefreshRate m_refreshRate;            // Refresh rate saved config

    /**
     * @brief RTOS variables:
     */
    SemaphoreHandle_t m_frameRequestSmphr;  // Semaphore: request a new frame
    SemaphoreHandle_t m_dataReadySmphr;     // Semaphore: received frame is ready for handling
    const uint32_t m_TaskTimeoutMs = 1000;  // Waiting timeout in milliseconds

    /**
     * @brief I2C Xfer Complete ISR callback
     */
    void run(uint16_t, uint32_t) override;

    /**
     * @brief Error (task timeout) handler
     */
    void faultHandler();

    /**
     * @brief New frame request task
     * @details RTOS task and subtask
     */
    static void requestDataTask(void*);
    void requestDataRoutine();

    /**
     * @brief Received data handling task
     * @details RTOS task and subtask
     */
    static void calcDataRoutine(void*);
    void calculatingRoutine();
};

}  // namespace task
