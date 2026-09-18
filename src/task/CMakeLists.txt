aux_source_directory(./ srcDir)
aux_source_directory(./tasks srcDir)
get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_library(${LIB_NAME} STATIC ${srcDir})

target_link_libraries(${LIB_NAME} freertos_kernel)
