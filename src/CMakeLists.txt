# Macro for low-leveled CMakeLists:
macro(subdirlist result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child} AND EXISTS ${curdir}/${child}/CMakeLists.txt)
        set(dirlist ${dirlist} ${child})
    endif()
    endforeach()
  set(${result} ${dirlist})
endmacro()

add_subdirectory(thirdParty)
add_subdirectory(task)
add_subdirectory(driver)
add_subdirectory(libs)
add_subdirectory(modules)
add_subdirectory(platform)
add_subdirectory(terminal)
add_subdirectory(utility)

# For FreeRTOS functions implementations
if (WITH_RTOS)
  add_library(rtosStatic STATIC FreeRTOSStatic.cpp) 
endif()

target_link_libraries(${PROJECT_NAME}.elf modules driver fatfs platform terminal utility freertos_kernel rtosStatic task sensorlib graphic)
