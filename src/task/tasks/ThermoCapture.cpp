#include "ThermoCapture.h"
#include "driver/i2c/I2c.h"
#include "driver/Dwt.h"
#include "Loger.h"
#include "task/TaskData.h"

namespace task
{

ThermoMatrixManager::ThermoMatrixManager(driver::I2C* i2c, uint16_t devAddr) : m_i2c(i2c),
                                                                               m_i2cAddr(devAddr),
                                                                               m_thermoDriver(devAddr)
{
    m_thermoDriver.setI2cTx(thermomatrix::Mlx90640::I2C_Tx::create<driver::I2C, &driver::I2C::masterTx>(*m_i2c));
    m_thermoDriver.setI2cTxRx(thermomatrix::Mlx90640::I2C_TxRx::create<driver::I2C, &driver::I2C::masterTxRx>(*m_i2c));
    m_thermoDriver.setI2cRxDMA(thermomatrix::Mlx90640::I2C_RxDMA::create<driver::I2C, &driver::I2C::readRegViaDMA_16bitAddr>(*m_i2c));

    /* Connect the I2C Data Xfer Ready (for DMA mode) signal */
    //m_i2c->xferCmpltSubscribeISR(this); // TODO: CHECK AND DEBUG if it works HERE!
}

void ThermoMatrixManager::setMeasConsumer(SlotInterface<const ThermoArray*>*slot)
{
    if (slot)
        m_sendReadyData.connect(slot);
}

void ThermoMatrixManager::setPowerCtrl(SlotInterface<bool>*slot)
{
    if (slot)
        m_sensorPower.connect(slot);
}

bool ThermoMatrixManager::init(thermomatrix::Mlx90640::RefreshRate refreshRate)
{
    /* Connect the I2C Data Xfer Ready (for DMA mode) signal */
    m_i2c->xferCmpltSubscribeISR(this);

    /* Powering on (if wasn't) */
    m_sensorPower.activ(true);

    /* Init the device */
    m_refreshRate = refreshRate;
    if (!m_thermoDriver.init(m_refreshRate))
        return false;

    if (!m_thermoDriver.readParameters())
        return false;

    if (m_dataReadySmphr == NULL)
        m_dataReadySmphr = xSemaphoreCreateBinary();
    if (m_frameRequestSmphr == NULL)
        m_frameRequestSmphr = xSemaphoreCreateBinary();
    if (!m_dataReadySmphr || !m_frameRequestSmphr)
        return false;

    m_state = States::Initialised;

    /* RTOS Tasks initialisation: */
    BaseType_t calcTask = xTaskCreate(calcDataRoutine, "90640:Handler", task::StackSizes[Tasks::SensorCalcTask], this, task::SensorCalcTaskPriority, &task::tasksHandlers[Tasks::SensorCalcTask]);
    BaseType_t requestTask = xTaskCreate(requestDataTask, "90640:Requester", task::StackSizes[Tasks::SensorReadTask], this, task::SensorReadTaskPriority, &task::tasksHandlers[Tasks::SensorReadTask]);
    if (calcTask == pdPASS && requestTask == pdPASS)
    {
        xSemaphoreGive(m_frameRequestSmphr);
        return true;
    }

    Log(lmTask, Error) << "Mlx90640 task initialisation FAILED";
    return false;
}

void ThermoMatrixManager::faultHandler()
{
    /* Device hard reset */
    m_sensorPower.activ(false);
    driver::DwtTimer::getInstance().delayMs(200);
    m_sensorPower.activ(true);

    /* Re-initialisation */
    if (m_thermoDriver.init(m_refreshRate))
        Log(lmTask, Info) << "Mlx90640 successfully reinitialised";
    else
        Log(lmTask, Error) << "Mlx90640 reinitialisation FAILED";
}

void ThermoMatrixManager::run(uint16_t devAddr, uint32_t)
{
    /* --------------- I S R --------------- */

    if (devAddr != m_i2cAddr)
        return;

    /* It is ISR, so here only short operations... */
    switch (m_state)
    {
    case States::FrameRequested:
        /* Pixels data received, request the AUX data */
        m_state = m_thermoDriver.auxRequest() ? States::AuxRequested : States::Error;
        break;
    case States::AuxRequested:
        /* AUX data received, request Control Register value */
        m_state = m_thermoDriver.controlRequest() ? States::CtrlRequested : States::Error;
        break;
    case States::CtrlRequested:
        /* All data collected:  */
        {
            m_state = States::DataHandling;
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            if (m_dataReadySmphr)
            {
                xSemaphoreGiveFromISR(m_dataReadySmphr, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
        break;
    }
}

void ThermoMatrixManager::calculatingRoutine()
{
    if (xSemaphoreTake(m_dataReadySmphr, pdMS_TO_TICKS(m_TaskTimeoutMs)) == pdTRUE)
    {
        if (m_thermoDriver.dataHandle())  // Both subpages have calculated
        {
            m_sendReadyData.activ(m_thermoDriver.getMeasurements());
        }
        xSemaphoreGive(m_frameRequestSmphr); // Give semaphore for next request task
    }
    else
        faultHandler();
}

void ThermoMatrixManager::requestDataRoutine()
{
    if (xSemaphoreTake(m_frameRequestSmphr, pdMS_TO_TICKS(m_TaskTimeoutMs)) == pdTRUE)
    {
        /* Wait for new thermo frame to be ready^ */
        auto timeoutParam = driver::DwtTimer::getInstance().timeoutInit(500);
        while (!m_thermoDriver.isFrameReady())
        {
            if (driver::DwtTimer::getInstance().checkTimeout(timeoutParam))
            {
                faultHandler();
                return;
            }
        }
        /* Request new termo frame: */
        if (m_thermoDriver.frameRequest())
            m_state = States::FrameRequested;
        else
            faultHandler();
    }
    else
        faultHandler();
}

void ThermoMatrixManager::calcDataRoutine(void* pvParameters)
{
    ThermoMatrixManager* const instance = static_cast<ThermoMatrixManager*>(pvParameters);
    if (instance)
    {
        for (;;)
        {
            instance->calculatingRoutine();
        }
    }
    vTaskDelete(nullptr); 
}

void ThermoMatrixManager::requestDataTask(void* pvParameters)
{
    ThermoMatrixManager* instance = static_cast<ThermoMatrixManager*>(pvParameters);
    if (instance)
    {
        for (;;)
        {
            instance->requestDataRoutine();
        }
    }
    vTaskDelete(nullptr); 
}

}  // namespace task
