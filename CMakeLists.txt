cmake_minimum_required(VERSION 3.17)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# PROJECT SETTINGS
project(STM32H743_BASE C CXX ASM)
add_definitions(-DSTM32H743xx)

# PROJECT OPTIONS:
option(WITH_ETL "if enabled, build with ELT library" ON)    # For some submodules..
option(WITH_RTOS "if enabled, build with RTOS option" ON)

# INCLUDE PATHS:
if (WITH_ETL)   # ETL needs a bit more RAM than STD
    add_definitions(-DWITH_ETL)
    include_directories(src/thirdParty/etl/include)
endif()

if (WITH_RTOS)
    add_definitions(-DWITH_RTOS)
    include_directories(src/thirdParty/rtos/include)
    include_directories(src/thirdParty/rtos/portable/GCC/ARM_CM4F) # for portmacro.h
    include_directories(src/task)
endif()

include_directories(.)
include_directories(src)
include_directories(src/libs)
include_directories(src/thirdParty)
include_directories(src/modules/include)
include_directories(src/libs/graphiclib/include)
include_directories(src/libs/sensorlib/include)
include_directories(CMSIS/Device/ST/STM32H7xx/Include CMSIS/Include)


# extern quartz frequency
add_definitions(-DHSE_VALUE=8000000)
#add_definitions(-DHSE_VALUE=25000000)

set(MCU     -mthumb 
            -mcpu=cortex-m7 
            -mfloat-abi=hard 
            -mfpu=fpv5-sp-d16 
            -specs=nosys.specs
            -specs=nano.specs
            -u _printf_float)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-O0)
endif ()

add_compile_options (-Wno-register)
set(SOURCES_STARTUP main.cpp 
                    startup_stm32h743xx.s
                    system_stm32h7xx.c)

add_compile_options (${MCU})
add_compile_options (-fdata-sections)
add_compile_options (-ffunction-sections)
add_compile_options (-fno-exceptions)       # Exeptions disable (saves a lot of Flash)
add_definitions(-Wno-narrowing)             # Disable errors of using char as unsigned
add_definitions(-Wno-psabi)

add_link_options (${MCU}
    -lc
    -lm
    -lnosys
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections)

add_link_options(-T ${CMAKE_SOURCE_DIR}/STM32H743VITx_FLASH.ld)
add_link_options(-Wl,--print-memory-usage)

add_executable(${PROJECT_NAME}.elf ${SOURCES_STARTUP} ${SOURCE_FILES})
set(GIT_DIR_LOOKUP_POLICY ALLOW_LOOKING_ABOVE_CMAKE_SOURCE_DIR)

add_subdirectory(src)

add_custom_command(TARGET "${PROJECT_NAME}.elf" POST_BUILD
        # Build .hex and .bin files
        COMMAND ${CMAKE_OBJCOPY} -Obinary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_OBJCOPY} -Oihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex        
        # Display sizes
        #COMMAND arm-none-eabi-size --format=berkeley ${PROJECT_NAME}.elf
        )
