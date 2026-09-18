#include "ThermoScreen.h"
#include "task/TaskData.h"
#include <etl/algorithm.h>
#include <etl/utility.h>
#include "driver/Dwt.h"
#include "Loger.h"
#include "Converters.h"

using namespace graphic;

/* Iron Bow palette look-up table */
static const uint16_t Palette_Ironbow[256]
{
    0x0000, 0x0000, 0x0001, 0x0001, 0x0002, 0x0002, 0x0003, 0x0003, 0x0004, 0x0004, 0x0005, 0x0005, 0x0006, 0x0006, 0x0007, 0x0008,
    0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x1017,
    0x1817, 0x2017, 0x2816, 0x3016, 0x3816, 0x4015, 0x4815, 0x5015, 0x5814, 0x6014, 0x6814, 0x7013, 0x7813, 0x8013, 0x8812, 0x8812,
    0x9011, 0x9811, 0xa010, 0xa810, 0xb00f, 0xb80f, 0xc00e, 0xc00e, 0xc80d, 0xd00d, 0xd80c, 0xe00c, 0xe80b, 0xf00b, 0xf80a, 0xf80a,
    0xf80a, 0xf809, 0xf809, 0xf809, 0xf808, 0xf808, 0xf808, 0xf808, 0xf807, 0xf807, 0xf807, 0xf806, 0xf806, 0xf806, 0xf805, 0xf805,
    0xf805, 0xf805, 0xf804, 0xf804, 0xf804, 0xf803, 0xf803, 0xf803, 0xf802, 0xf802, 0xf802, 0xf802, 0xf801, 0xf801, 0xf801, 0xf800,
    0xf800, 0xf820, 0xf840, 0xf860, 0xf880, 0xf8a0, 0xf8c0, 0xf8e0, 0xf900, 0xf920, 0xf940, 0xf960, 0xf980, 0xf9a0, 0xf9c0, 0xf9e0,
    0xfa00, 0xfa20, 0xfa40, 0xfa60, 0xfa80, 0xfaa0, 0xfac0, 0xfae0, 0xfb00, 0xfb20, 0xfb40, 0xfb60, 0xfb80, 0xfba0, 0xfbc0, 0xfbe0,
    0xfc00, 0xfc20, 0xfc40, 0xfc60, 0xfc80, 0xfca0, 0xfcc0, 0xfce0, 0xfd00, 0xfd20, 0xfd40, 0xfd60, 0xfd80, 0xfda0, 0xfdc0, 0xfde0,
    0xfe00, 0xfe20, 0xfe40, 0xfe60, 0xfe80, 0xfea0, 0xfec0, 0xfee0, 0xff00, 0xff20, 0xff40, 0xff60, 0xff80, 0xffa0, 0xffc0, 0xffe0,
    0xffe1, 0xffe2, 0xffe3, 0xffe4, 0xffe5, 0xffe6, 0xffe7, 0xffe8, 0xffe9, 0xffea, 0xffeb, 0xffec, 0xffed, 0xffee, 0xffef, 0xfff0,
    0xfff1, 0xfff2, 0xfff3, 0xfff4, 0xfff5, 0xfff6, 0xfff7, 0xfff8, 0xfff9, 0xfffa, 0xfffb, 0xfffc, 0xfffd, 0xfffe, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff
};

inline uint16_t getTemperatureColor(float temp, float tMin, float tMax)
{
    // Out of range protection:
    if (temp <= tMin) return Palette_Ironbow[0];
    if (temp >= tMax) return Palette_Ironbow[255];
    // Index calculation:
    uint8_t index = static_cast<uint8_t>((temp - tMin) * (255.0f / (tMax - tMin)));
    return Palette_Ironbow[index];
}

TermoScreen::TermoScreen(graphic::GraphicBuilder& display) : 
    m_timer(500),
    m_display(display),
    m_pictureStartPoint{32, 32},
    m_pictureEndPoint{m_pictureStartPoint.x + m_ImgCoilsPixSize - 1, m_pictureStartPoint.y + m_ImgRowsPixSize - 1}
{
    m_timer.subscribeHandler(rtos::Timer::Handler::create<TermoScreen, &TermoScreen::timerMetaPrintCallback>(*this));
}

void TermoScreen::run(const thermomatrix::ThermoArray* rxPack, uint32_t)
{
    /* Cross-Task exchange: get a new pack of calculated thermo data: */
    if (rxPack)
        xQueueSend(m_measuresQueue, rxPack, portMAX_DELAY);
}

bool TermoScreen::tasksInit()
{
    m_measuresQueue = xQueueCreate(1, sizeof(thermomatrix::ThermoArray));
    mainBufFreeSemphr = xSemaphoreCreateBinary();
    if (m_measuresQueue && mainBufFreeSemphr)
    {
        xSemaphoreGive(mainBufFreeSemphr);
        m_timer.start();
        BaseType_t result = xTaskCreate(pictureBuildTask, "Screen builder", task::StackSizes[task::Tasks::DisplayUpdate], this, task::DisplayUpdatePriority, &task::tasksHandlers[task::Tasks::DisplayUpdate]);
        return (result == pdPASS);
    }
    else
    {
        Log(lmTask, Error) << "TermoScreen queue creeation FAILED";
        return false;
    }
}

void TermoScreen::imageUpdate(const thermomatrix::ThermoArray& newThermData)
{
    /* Find min and max temperature values: */
    auto result = etl::minmax_element(&newThermData.data[0], &newThermData.data[newThermData.size-1]);
    m_minVal = *result.first;
    m_maxVal = *result.second;

    uint16_t coilCounter = 0;
    uint16_t xStart = 0;
    uint16_t yStart = 0;

    /* Fill the Image Buffer with pixels values */
    for (int ind = 0; ind < newThermData.size; ind++)
    {
        // Get pixel colour code:
        rgb565 pixel = getTemperatureColor(newThermData.data[ind], m_minVal, m_maxVal);

        // Fill the rectangle (one point) area:
        for (int i = 0; i < m_ImgPointSideSize; i++)
        {
            for (int j = 0; j < m_ImgPointSideSize; j++)
            {
                m_imgBuffer[yStart + j][xStart + i] = pixel;
            }
        }
        // Shift start position:
        xStart += m_ImgPointSideSize;
        // Switch to the new row:
        if (xStart >= m_ImgCoilsPixSize)
        {
            xStart = 0;
            yStart += m_ImgPointSideSize;
        }
    }

    /* Make the Upper Layer (cross center aiming point): */
    for (int i = 0; i < m_ImgCoilsPixSize; i++)
    {
        m_imgBuffer[m_ImgRowsPixSize / 2 - 1][i] = RGB565Colours::RGB565_Black;
        m_imgBuffer[m_ImgRowsPixSize / 2][i] = RGB565Colours::RGB565_Black;
    }
    for (int i = 0; i < m_ImgRowsPixSize; i++)
    {
        m_imgBuffer[i][m_ImgCoilsPixSize / 2 - 1] = RGB565Colours::RGB565_Black;
        m_imgBuffer[i][m_ImgCoilsPixSize / 2] = RGB565Colours::RGB565_Black;
    }

    /* Start data uploading to the display: */
    m_display.locateArea(m_pictureStartPoint, m_pictureEndPoint);
    m_display.sendPixels((uint16_t*)m_imgBuffer, m_ImgCoilsPixSize * m_ImgRowsPixSize);
}

void TermoScreen::printString(Coordnt begin, const char* str, uint16_t size, const Font* font, rgb565 fontClr, rgb565 bkgrClr)
{
    if (!str || !font || size == 0)
        return;

    const uint32_t pixelcount = font->FontHeight * font->FontWidth;
    Coordnt leftUp = begin;
    Coordnt rightDwn
    {
        .x = leftUp.x + font->FontWidth - 1,
        .y = leftUp.y + font->FontHeight - 1
    };

    // Check the validity of coordinates (do they fit the to screen space):
    if (rightDwn.x >= m_display.getScreenWidth() || rightDwn.y >= m_display.getScreenHeight())
        return;
    
    while (size > 0)
    {
        // Waiting for the buffer to be released (we can't use it before finishing last DMA process):
        if (xSemaphoreTake(mainBufFreeSemphr, pdMS_TO_TICKS(1000)) != pdTRUE)
            return;
        
        // Prepare the character pixels data in buffer and start the DMA transferation:
        graphic::GraphicBuilder::makeBufferedChar((uint16_t*)m_imgBuffer, 200, *str, font, fontClr, bkgrClr);
        m_display.locateArea(leftUp, rightDwn);
        m_display.sendPixels((uint16_t*)m_imgBuffer, pixelcount);

        // Offset and check available screen space:
        leftUp.x += font->FontWidth;
        rightDwn.x += font->FontWidth;
        if (rightDwn.x >= m_display.getScreenWidth())
            return; // Out of the screen space

        // Go to the next symbol:
        str++;
        size--;
    }
}

void TermoScreen::metaDataUpdate(const thermomatrix::ThermoArray& newThermData)
{
    /* Calculate metadata: */
    m_centerPointTemp = (newThermData.data[367] + newThermData.data[368] + newThermData.data[399] + newThermData.data[400]) / 4.0;

    /* Print the metadata strings */
    const graphic::Font *font = &font11x18;
    char strBuff[10]{0};
    Coordnt startPoint {32 + 14 * font->FontWidth, 232}; // Offset by 14 chars of head string "Center point: "

    uint32_t strSize = conversions::FloatToString(m_centerPointTemp, strBuff, 1);
    memcpy(m_stringBuffer, strBuff, strSize);
    printString(startPoint, m_stringBuffer, strSize, font, RGB565_Green, RGB565_Black);

    strSize = conversions::FloatToString(m_minVal, strBuff, 1);
    startPoint = {32 + 7 * font->FontWidth, 255};    // Second string. Offset by 7 chars of head string "Center point: "
    memcpy(m_stringBuffer, strBuff, strSize);
    printString(startPoint, m_stringBuffer, strSize, font, RGB565_Green, RGB565_Black);

    startPoint.x += strSize * font->FontWidth;
    printString(startPoint, " to ", strSize, font, RGB565_Green, RGB565_Black);
    
    strSize = conversions::FloatToString(m_maxVal, strBuff, 1);
    startPoint.x += 4 * font->FontWidth;
    memcpy(m_stringBuffer, strBuff, strSize);
    printString(startPoint, m_stringBuffer, strSize, font, RGB565_Green, RGB565_Black);
}

void TermoScreen::spiXferFinishHandler()
{
    /* --------------- I S R --------------- */

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(mainBufFreeSemphr, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void TermoScreen::pictureBuildRoutine()
{
    if (xQueueReceive(m_measuresQueue, &m_receivedThermo, portMAX_DELAY) == pdPASS)
    {
        /* 1. Incoming new measurements: calculate and print new frame: */
        imageUpdate(m_receivedThermo);
        /* 2. By timer: update and print the metadata (measurements values): */
        if (metaPrintTime)
        {
            metaDataUpdate(m_receivedThermo);
            metaPrintTime = false;
        }
    }
}

void TermoScreen::pictureBuildTask(void* pvParameters)
{
    TermoScreen* instance = static_cast<TermoScreen*>(pvParameters);
    if (instance)
    {
        /* Startup initialisation: print some info strings: */
        
        instance->printString({32, 232}, "Center point: ", 14, &font11x18, RGB565_Green, RGB565_Black);
        instance->printString({32, 255}, "Range: ", 7, &font11x18, RGB565_Green, RGB565_Black);
        /* Entering the task loop: */
        for (;;)
        {
           instance->pictureBuildRoutine();
        }
    }
    vTaskDelete(nullptr); 
}

void TermoScreen::timerMetaPrintCallback()
{
    UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    metaPrintTime = true;
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}
