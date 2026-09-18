aux_source_directory(./ srcDir)
get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_library(${LIB_NAME} STATIC ${srcDir})