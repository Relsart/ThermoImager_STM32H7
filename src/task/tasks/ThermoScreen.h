#pragma once

#include <stdint.h>
#include "Signal.h"
#include "Mlx90640.h"
#include "libs/graphiclib/include/BuilderInterface.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include "task/TimerWrapper.h"

using namespace graphic;

/**
 * @brief Option for linear interpolation rendering type (smoothing image)
 * @details ACHTUNG: For the processor time optimisation it works with powers-of-two scales (m_ImgScale)!
 */
#define INTERPOLATION_RENDER

/**
 * @brief Thermo picture builder
 * 
 * @details SLOT for MLX90640 measurements data struct
 * 
 *  Display size is 480*320 (in portrait mode)
 *  IR sensor (MLX90640) is 32*24 pixels
 *  Thermo picture is 256*192 (one point is 8*8 pixels)
 */
class TermoScreen : public SlotInterface <const thermomatrix::ThermoArray*>
{
public:
    /**
     * @brief Constructor
     * @param [in] display ST7xx display driver
     */
    TermoScreen(graphic::GraphicBuilder& display);

    /**
     * @brief RTOS tasks initialisation
     */
    bool tasksInit();

    /**
     * @brief Handler callback of SPI End Of Transfer interruption
     * @details Callback, part of ISR
     */
    void spiXferFinishHandler();

private:
    graphic::GraphicBuilder& m_display;     // Display middle-layer driver
    rtos::Timer m_timer;                    // Timer for metadata updating
    bool metaPrintTime = false;             // Flag for metadata updating (sets by Timer)
    QueueHandle_t m_measuresQueue;          // Ready measurements data queue
    SemaphoreHandle_t mainBufFreeSemphr;    // Semaphore: Main Pixels Buffer is free (no DMA process uses it) 
    
    /* Screen coodrinates variables: */
    static const uint16_t m_MatrixCoils = MLX90640_COLUMN_NUM;              // Number of thermomatrix coils
    static const uint16_t m_MatrixRows = MLX90640_LINE_NUM;                 // Number of thermomatrix rows
    static const uint16_t m_ImgScale = 8;                                   // Size of one logical thermoframe pixel (in screen pixels)
    static const uint16_t m_ImgCoilsPixSize = m_ImgScale * m_MatrixCoils;   // Number of coils in thermoframe (in screen pixels) 256
    static const uint16_t m_ImgRowsPixSize = m_ImgScale * m_MatrixRows;     // Number of rows in thermoframe (in screen pixels) 192
    const graphic::Coordnt m_pictureStartPoint;     // Coordinates of thermo picture rectangle start (upper left cornen)
    const graphic::Coordnt m_pictureEndPoint;       // Coordinates of thermo picture rectangle end (lower right cornen)

    /* Buffers */
    uint16_t m_imgBuffer[m_ImgRowsPixSize][m_ImgCoilsPixSize]{0};   // Buffer for preparing and sending display data (as it uses DMA for speed)
    char m_stringBuffer[100]{0};                    // Buffer for output strings data
    thermomatrix::ThermoArray m_receivedThermo;     // Thermo measurements, received from Matrix sensor

    /* Saved measurements */
    float m_centerPointTemp;    // Averaged temperature of central 4 pixels 
    float m_minVal = 0;         // Saved min value of temperature (for the current scan)
    float m_maxVal = 0;         // Saved max value of temperature (for the current scan)

    /**
     * @brief Thermo data handling (transfer to Picture Builder Task)
     * @param [in] rxPack Ready Measurements Pack address 
     */
    void run(const thermomatrix::ThermoArray* rxPack, uint32_t) override;

    /**
     * @brief Rebuild the screen picture for new measurements
     * @details RTOS task and subtask
     */
    static void pictureBuildTask(void*);
    void pictureBuildRoutine();

    /**
     * @brief Command for the metadata printing
     * @details Callback for the timer
     */
    void timerMetaPrintCallback();

    /**
     * @brief Calculates temperature values to colours and updates the Thermo frame picture
     * @details RTOS task functional (subtask)
     * @param [in] newThermData Pack of new measurements from the thermo-array sensor
     */
    void imageUpdate(const thermomatrix::ThermoArray& newThermData);

    /**
     * @brief Simple image rendering (colour squares with m_ImgScale side)
     * @param [in] newThermData Link to the new measurements pack
     */
    void simpleRender(const thermomatrix::ThermoArray& newThermData);

    /**
     * @brief Image rendering with linear interpolation algorithm
     * @details m_ImgScale must be a powers-of-two (2, 4, 8...)
     * @param [in] newThermData Link to the new measurements pack
     */
    void interpolationRender(const thermomatrix::ThermoArray& newThermData);

    /**
     * @brief Converts the metadata (measurements) to strings and outputs to the display
     * @details RTOS task functional (subtask)
     * @param [in] newThermData Pack of new measurements from the thermo-array sensor
     */
    void metaDataUpdate(const thermomatrix::ThermoArray& newThermData);

    /**
     * @brief Converts the metadata (measurements) to strings and outputs to the display
     * @param [in] begin String start coordinates
     * @param [in] str String
     * @param [in] size String length
     * @param [in] font Font data
     * @param [in] fontClr Font colour
     * @param [in] bkgrClr Background colour
     */
    void printString(Coordnt begin, const char* str, uint16_t size, const Font* font, rgb565 fontClr, rgb565 bkgrClr);
};
