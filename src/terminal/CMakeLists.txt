#aux_source_directory(./ srcDir)
set (srcDir CmdHardLog.cpp CmdSD.cpp Functions.cpp)
get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

if (WITH_RTOS)
    list(APPEND srcDir CmdTask.cpp)
endif()

add_library(${LIB_NAME} STATIC ${srcDir})

target_link_libraries(${LIB_NAME} driver)
target_link_libraries(${LIB_NAME} fatfs)
