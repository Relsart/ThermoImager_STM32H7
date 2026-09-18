# Thermo Imager prototype project for MLX90640 on STM32H7 MCU

## Description

This project captures a 32x24 thermal frame, processes the data and renders a real-time thermal heatmap on an LCD display.
The calculated temperature information at the central point and the entire temperature range of the current frame is also displayed.

## Installation & Usage

Clone the repository:
   ```bash
   git clone --recurse-submodules https://gitlab.com/Relsart/thermo_imager.git
   ```
It in necessary to pull all dependent submodules: sensors, graphic libraries, stm32 modules and external ETL library.

## Hardware Requirements

*   **Microcontroller:** STM32 (e.g., STM32F7. It was successfully tested on STM32H743 Nucleo board)
*   **Thermal Sensor:** Melexis MLX90640 (32x24 IR array)
*   **Display:** SPI LCD ST77xx series (e.g. ST7796 320x480 TFT display)
*   **Power Supply:** 3.3V stable source (isolated from noisy digital lines for better sensor accuracy)

## Software Architecture & Stack

*   **Language:** C / C++
*   **Framework/IDE:** Microsoft VSCode
*   **Toolchains:** Cmake and GCC arm-none-eabi toolchain (tested well at 10.3.1 version).
*   **Drivers:** MLX90640 Official APIs
*   **Key Optimizations:**
    *   **RTOS:**  FreeRTOS is used to optimally distribute processor time between the tasks of reading, processing and displaying thermo information on the screen.
    *   **I2C with DMA:** Non-blocking background reading of the MLX90640 RAM frames.
    *   **SPI with DMA:** Fast screen rendering to prevent display stuttering.
    *   **Buffered image data:** Used for increasing image updating speed on the display (since the SPI bandwidth is quite low).

### Wiring Diagram (Typical I2C/SPI setup, see Schematic.pdf)

| STM32 Pin | Periphery | Component Pin |
| :--- | :--- | :--- |
| **I2C_SCL (PF1)** | MLX90640 | SCL |
| **I2C_SDA (PF0)** | MLX90640 | SDA |
| **3V3 / GND**| Power | VCC / GND |
| **SPI_SCK (PA5)** | LCD Display | CLK |
| **SPI_MOSI (PA7)**| LCD Display | MOSI |
| **GPIO (PA1)** | LCD Display | LED |
| **GPIO (PB1)** | LCD Display | RS (D/C) |
| **GPIO (PB14)** | LCD Display | RST |
| **GPIO (PB12)** | LCD Display | SPI_CS |
| **UART_TX (PD5)** | Debug console | Rx |
| **UART_RX (PD6)** | Debug console | Tx |

## Using external libraries

- [ ] Etl library (https://github.com/ETLCPP/etl.git)
- [ ] FreeRTOS (https://github.com/FreeRTOS)