if(WITH_RTOS)
    # Add a freertos_config library:
    add_library(freertos_config INTERFACE)
    target_include_directories(freertos_config SYSTEM INTERFACE "rtos/include")
    # Select the heap port.  values between 1-4 will pick a heap.
    set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
    # Select the native compile PORT
    set(FREERTOS_PORT "GCC_ARM_CM4F" CACHE STRING "" FORCE)
    add_subdirectory(rtos)
endif()

add_subdirectory(etl)
add_subdirectory(fatfs)
