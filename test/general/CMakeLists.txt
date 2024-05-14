add_library(general_test)
target_sources(general_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/argparse.c" 
    "${CMAKE_CURRENT_SOURCE_DIR}/debug.c"
)
target_include_directories(general_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc/")